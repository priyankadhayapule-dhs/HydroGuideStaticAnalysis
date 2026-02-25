//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "PhraseMap"
#include <ThorUtils.h>
#include <AI/Guidance/PhraseMap.h>
#include <imgui.h>

PhraseMap::PhraseMap() :
    mPhrases(nullptr),
    mPhraseNames(),
    mLocaleMap()
{}

void PhraseMap::loadLocaleMap(const Json::Value &localeMap) {
    for (auto locale = localeMap.begin(); locale != localeMap.end(); ++locale) {
        // locale.key() is the locale name
        // *locale is a map of phrase name -> localized phrase text
        std::vector<std::string> localizedPhrases;
        for (auto entry = locale->begin(); entry != locale->end(); ++entry) {
            PhraseId phrase = addPhrase(entry.key().asCString());
            if (localizedPhrases.size() <= phrase.id) {
                localizedPhrases.resize(phrase.id+1);
            }
            localizedPhrases[phrase.id] = entry->asString();
        }
        mLocaleMap.push_back(std::make_pair(locale.key().asString(), std::move(localizedPhrases)));
    }

    // Set default locale to en
    setLocale("en");
}

PhraseId PhraseMap::addPhrase(const char *name) {
    auto iter = std::find(mPhraseNames.begin(), mPhraseNames.end(), name);
    if (iter != mPhraseNames.end()) {
        // Found existing phrase
        return PhraseId{static_cast<unsigned int>(iter-mPhraseNames.begin())};
    } else {
        // Add new phrase
        mPhraseNames.emplace_back(name);
        return PhraseId{static_cast<unsigned int>(mPhraseNames.size()-1)};
    }
}

void PhraseMap::setLocale(const char *locale) {
    for (unsigned int i=0; i != mLocaleMap.size(); ++i) {
        if (mLocaleMap[i].first == locale) {
            mPhrases = &mLocaleMap[i].second;
            return;
        }
    }
    LOGE("Failed to find locale %s", locale);
}

const std::string &PhraseMap::getPhraseText(PhraseId phrase) const {
    //assert(mPhrases != nullptr);
    static const std::string EMPTY = std::string("");
    if (phrase == PhraseId::none()) {
        return EMPTY;
    }
    if (phrase.id >= mPhrases->size()) {
        LOGE("PhraseId %u not found in current phrase map (id too large)", phrase.id);
        return EMPTY;
    }
    const std::string &text = (*mPhrases)[phrase.id];
    if (text.empty()) {
        LOGE("PhraseId %u not found in current phrase map (text is empty)", phrase.id);
        return EMPTY;
    }
    return text;
}

void PhraseMap::addPhraseTextRanges(ImFontGlyphRangesBuilder *builder) {
    for (const auto &entry : mLocaleMap) {
        for (const std::string &phrase : entry.second) {
            builder->AddText(phrase.c_str(), phrase.c_str()+phrase.size());
        }
    }
    // Also always add basic ASCII set
    builder->AddText("abcdefghijklmnopqrstuvwxyz");
    builder->AddText("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    builder->AddText("0123456789");
    builder->AddText("-+!@#$%^&*()`~?/\\{}[]=_\"';:,.<>|");
}
