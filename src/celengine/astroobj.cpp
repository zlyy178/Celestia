
#include <celutil/debug.h>
#include <celutil/util.h>
#include "parseobject.h"
#include "astroobj.h"
#include "category.h"

AstroObject::AstroObject(AstroObject &o)
{
    setIndex(o.getIndex());

    auto cats = o.getCategories();
    if (cats != nullptr)
        for (auto *cat : *cats)
            addToCategory(cat);
}

AstroObject::AstroObject(AstroObject &&o)
{
    setIndex(o.getIndex());

    auto cats = o.getCategories();
    if (cats != nullptr)
        for (auto *cat : *cats)
            addToCategory(cat);
    o.clearCategories();
}

AstroObject::~AstroObject()
{
    if (getIndex() != AstroCatalog::InvalidIndex)
        freeIndexNumber(getIndex());

    clearCategories();
}

void AstroObject::setIndex(AstroCatalog::IndexNumber nr)
{
    if (m_mainIndexNumber != AstroCatalog::InvalidIndex)
        DPRINTF(LOG_LEVEL_WARNING, "AstroObject::setIndex(%u) on object with already set index: %u!\n", nr, m_mainIndexNumber);
    if (m_mainIndexNumber != nr)
    {
        if (m_mainIndexNumber != AstroCatalog::InvalidIndex)
            freeIndexNumber(nr);
        m_mainIndexNumber = nr;
        if (nr != AstroCatalog::InvalidIndex)
            assignIndexNumber(nr, this);
    }
}

Selection AstroObject::toSelection()
{
    return Selection(this);
}

AstroCatalog::IndexNumber AstroObject::setNewAutoIndex()
{
    auto i = getNewAutoIndex();
    setIndex(i);
    return i;
}

bool AstroObject::_addToCategory(UserCategory *c)
{
    if (m_cats == nullptr)
        m_cats = new CategorySet;
    m_cats->insert(c);
    return true;
}

bool AstroObject::addToCategory(UserCategory *c)
{
    if (!_addToCategory(c))
        return false;
    return c->_addObject(toSelection());
}

bool AstroObject::addToCategory(const std::string &s, bool create, const std::string &d)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
    {
        if (!create)
            return false;
        else
            c = UserCategory::newCategory(s, nullptr, d);
    }
    return addToCategory(c);
}

bool AstroObject::_removeFromCategory(UserCategory *c)
{
    if (!isInCategory(c))
        return false;
    m_cats->erase(c);
    if (m_cats->empty())
    {
        delete m_cats;
        m_cats = nullptr;
    }
    return true;
}

bool AstroObject::removeFromCategory(UserCategory *c)
{
    if (!_removeFromCategory(c))
        return false;
    return c->_removeObject(toSelection());
}

bool AstroObject::removeFromCategory(const std::string &s)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return removeFromCategory(c);
}

bool AstroObject::clearCategories()
{
    bool ret = true;
    while(m_cats != nullptr)
    {
        UserCategory *c = *(m_cats->begin());
        if (!removeFromCategory(c))
            ret = false;
    }
    return ret;
}

bool AstroObject::isInCategory(UserCategory *c) const
{
    if (m_cats == nullptr)
        return false;
    return m_cats->count(c) > 0;
}

bool AstroObject::isInCategory(const std::string &s) const
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return isInCategory(c);
}

bool AstroObject::loadCategories(Hash *hash, DataDisposition disposition, const std::string &domain)
{
    if (disposition == DataDisposition::Replace)
        clearCategories();
    std::string cn;
    if (hash->getString("Category", cn))
    {
        if (cn.empty())
            return false;
        return addToCategory(cn, true, domain);
    }
    Value *a = hash->getValue("Category");
    if (a == nullptr)
        return false;
    ValueArray *v = a->getArray();
    if (v == nullptr)
        return false;
    bool ret = true;
    for (auto it : *v)
    {
        cn = it->getString();
        if (!addToCategory(cn, true, domain))
            ret = false;
    }
    return ret;
}

std::unordered_map<AstroCatalog::IndexNumber, AstroObject*> AstroObject::m_mainIndex;

AstroCatalog::IndexNumber AstroObject::m_autoIndex { MaxAutoIndex };
