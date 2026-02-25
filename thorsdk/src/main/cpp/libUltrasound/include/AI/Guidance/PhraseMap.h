//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include "GuidanceTypes.h"
#include <json/value.h>
#include <vector>
#include <string>
#include <utility>

class ImFontGlyphRangesBuilder;

class PhraseMap {
public:
    PhraseMap();

    /// Loads a locale mapping (as a JSON string)
    void loadLocaleMap(const Json::Value &locale);

    /// Adds a phrase by name.
    PhraseId addPhrase(const char *name);

    /// Sets the current locale
    void setLocale(const char *locale);

    /// Get the text for a given phrase (in the current locale)
    const std::string& getPhraseText(PhraseId id) const;

    void addPhraseTextRanges(ImFontGlyphRangesBuilder *builder);

private:
    /// PhraseId -> Phrase text for current locale
    std::vector<std::string> *mPhrases;
    /// PhraseId -> Phrase name (global across locales)
    std::vector<std::string> mPhraseNames;
    /// Map of locale name -> list of localized phrases for that locale
    /// Because the expected number of locales is small and we will not be switching often,
    /// it makes sense for this to be a "flat map" aka vector.
    std::vector<std::pair<std::string, std::vector<std::string>>> mLocaleMap;
};