#pragma once

#include <Arduino.h>

#include "books/BookCache.h"

class SelectedCachedBook
{
public:
    void select(const CachedBook& book)
    {
        snprintf(id, sizeof(id), "%s", book.id);
        snprintf(title, sizeof(title), "%s", book.title);
        status[0] = '\0';
    }

    void clear()
    {
        id[0] = '\0';
        title[0] = '\0';
    }

    const char* getId() const { return id; }
    const char* getTitle() const { return title; }
    const char* getStatus() const { return status; }

    void setStatus(const char* nextStatus)
    {
        snprintf(status, sizeof(status), "%s", nextStatus);
    }

private:
    char id[32] = {};
    char title[64] = {};
    char status[64] = {};
};
