//
// Copyright 2020 EchoNous Inc.
//
#define LOG_TAG "Filesystem"
#include <android/asset_manager.h>
#include <fstream>
#include <cassert>

// POSIX directory creation and iteration
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <Filesystem.h>
#include <ThorUtils.h>

Filesystem::Filesystem() :
    mAssetManager(nullptr),
    mAssetsDir(),
    mCacheDir(),
    mJsonReader(nullptr) {}

ThorStatus Filesystem::init(AAssetManager *assetManager, const char *appPath) {
    //assert(appPath);

    mAssetManager = assetManager;
    mAssetsDir = appPath;
    if (mAssetsDir.back() != '/')
        mAssetsDir.push_back('/');
    mCacheDir = mAssetsDir + "cache/";

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    builder["allowComments"] = true;
    mJsonReader.reset(builder.newCharReader());

    ThorStatus status;
    status = ensurePath(mCacheDir.c_str());

    return status;
}

ThorStatus Filesystem::readAsset(const char *name, std::vector<unsigned char> &buffer) {
    ThorStatus result;
    LOGD("Loading %s, assets dir = \"%s\", assetManager = %p", name, mAssetsDir.c_str(), mAssetManager);
    {
        // Try reading from file system first
        result = readFromFilesystem((mAssetsDir + name).c_str(), buffer);
        if (!IS_THOR_ERROR(result)) {
            LOGD("Read asset %s from filesystem, size = %zu bytes", name, buffer.size());
            return result;
        }
    }
    if (mAssetManager) {
        // Then try reading from asset manager
        result = readFromAssets(name, buffer);
        if (!IS_THOR_ERROR(result)) {
            LOGD("Read asset %s from assets, size = %zu bytes", name, buffer.size());
            return result;
        }
    }
    LOGE("Fail to load \"%s\" from either assets or filesystem.", name);
    return TER_MISC_OPEN_FILE_FAILED;
}

ThorStatus Filesystem::readCache(const char *name, std::vector<unsigned char> &buffer) {
    std::string path = mCacheDir + name;
    return readFromFilesystem(path.c_str(), buffer);
}

ThorStatus Filesystem::readAssetJson(const char *name, Json::Value &json) {
    ThorStatus status;
    std::vector<unsigned char> contents;
    status = readAsset(name, contents);
    if (IS_THOR_ERROR(status)) {
        return status;
    }

    return parseJson(name, contents, json);
}

ThorStatus Filesystem::readCacheJson(const char *name, Json::Value &json) {
    ThorStatus status;
    std::vector<unsigned char> contents;
    status = readCache(name, contents);
    if (IS_THOR_ERROR(status)) {
        return status;
    }

    return parseJson(name, contents, json);
}

ThorStatus Filesystem::writeCache(const char *name, const std::vector<unsigned char> &buffer) {
    std::string path = mCacheDir + name;
    std::string dirpath = path.substr(0, path.find_last_of('/'));
    ThorStatus status = ensurePath(dirpath.c_str());
    if (IS_THOR_ERROR(status))
        return status;

    auto file = std::ofstream(path);
    if (!file) {
        return TER_MISC_CREATE_FILE_FAILED;
    }
    const char *data = reinterpret_cast<const char*>(buffer.data());
    size_t size = buffer.size();
    file.write(data, size);
    if (!file) {
        return TER_MISC_WRITE_FAILED;
    }
    return THOR_OK;
}

ThorStatus Filesystem::listAssetDirectory(const char *dir, std::vector<std::string> &entries) {
    ThorStatus status;
    entries.clear();
    std::string dirfs = mAssetsDir + dir;

    status = listDirectoryAssetManager(dir, entries);
    // TODO: If error is due to directory not existing, then that's ok
    // Any other error is passed up
    status = listDirectoryFilesystem(dirfs.c_str(), entries);
    // TODO: If error is due to directory not existing, then that's ok
    // Any other error is passed up

    // Remove any directories that are common to both
    std::sort(entries.begin(), entries.end());
    auto end = std::unique(entries.begin(), entries.end());
    entries.erase(end, entries.end());

    return THOR_OK;
}

ThorStatus Filesystem::listCacheDirectory(const char *dir, std::vector<std::string> &entries) {
    std::string dirfs = mCacheDir + dir;
    return listDirectoryFilesystem(dirfs.c_str(), entries);
}

AAssetManager *Filesystem::getAAssetManager() const {
    return mAssetManager;
}

const std::string &Filesystem::getAssetDir() const {
    return mAssetsDir;
}
const std::string &Filesystem::getCacheDir() const {
    return mCacheDir;
}

ThorStatus Filesystem::readFromAssets(const char *name, std::vector<unsigned char> &buffer) {
    ThorStatus result;

    if (!mAssetManager) {
        LOGE("Cannot load from assets, no asset manager!");
        return TER_MISC_INVALID_FILE;
    }
    AAsset *asset = AAssetManager_open(mAssetManager, name, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open asset file: %s", name);
        return TER_MISC_OPEN_FILE_FAILED;
    }

    size_t length = AAsset_getLength64(asset);
    buffer.resize(length);
    auto *assetBuffer = (const unsigned char*) AAsset_getBuffer(asset);
    if (assetBuffer) {
        std::copy_n(assetBuffer, length, buffer.begin());
        result = THOR_OK;
    } else {
        LOGE("Failed to get buffer for asset %s", name);
        result = TER_MISC_OPEN_FILE_FAILED;
    }
    AAsset_close(asset);
    return result;
}

ThorStatus Filesystem::readFromFilesystem(const char *name, std::vector<unsigned char> &buffer) {
    auto file = std::ifstream(name);
    if (!file) {
        return TER_MISC_OPEN_FILE_FAILED;
    }
    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(length);
    file.read((char*)buffer.data(), length);
    if (!file) {
        LOGE("Failed to read from file");
        return TER_MISC_INVALID_FILE;
    }
    return THOR_OK;
}

ThorStatus Filesystem::makeDirectory(const char *dir, bool existOk) {
    int rc = ::mkdir(dir, S_IRWXU | S_IRWXG);
    if (rc != 0) {
        if (errno == EEXIST && existOk)
            return THOR_OK;
        return TER_MISC_CREATE_FILE_FAILED;
    }
    return THOR_OK;
}

ThorStatus Filesystem::ensurePath(const char *path) {
    ThorStatus status;
    std::string pathstr = path;
    size_t begin = 0;
    size_t end = pathstr.find_first_of('/');
    while (end != std::string::npos) {
        auto dir = pathstr.substr(0, end);
        makeDirectory(dir.c_str(), true);
        begin = end+1;
        end = pathstr.find_first_of('/', begin);
    }
    // Only check status of last one, in case we don't have permission for higher directories but they do exist
    status = makeDirectory(pathstr.c_str(), true);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error %08x in ensurePath", status);
        return status;
    }

    return THOR_OK;
}

ThorStatus Filesystem::listDirectoryAssetManager(const char *path, std::vector<std::string> &entries) {
    if (mAssetManager == nullptr) {
        return TER_MISC_PARAM;
    }

    AAssetDir *dir = AAssetManager_openDir(mAssetManager, path);
    if (!dir) {
        LOGE("AAssetManager_openDir returned null for path \"%s\"", path);
        return TER_MISC_OPEN_FILE_FAILED;
    }
    const char *entry = AAssetDir_getNextFileName(dir);
    while (entry != nullptr) {
        LOGD("Read asset directory entry %s", entry);
        entries.push_back(entry);
        entry = AAssetDir_getNextFileName(dir);
    }
    AAssetDir_close(dir);
    return THOR_OK;
}

ThorStatus Filesystem::listDirectoryFilesystem(const char *path, std::vector<std::string> &entries) {
    DIR *dir = ::opendir(path);
    if (dir == nullptr) {
        if (errno == ENOTDIR) {
            return TER_MISC_INVALID_FILE;
        } else {
            return TER_MISC_OPEN_FILE_FAILED;
        }
    }

    errno = 0;
    dirent *entry = readdir(dir);
    while (entry != nullptr) {
        if (entry->d_type == DT_REG) {
            // Only store regular files
            entries.push_back(entry->d_name);
        }
        entry = readdir(dir);
    }
    if (errno != 0) {
        LOGE("readdir error %d", errno);
        return TER_MISC_ABORT;
    }

    ::closedir(dir);
    return THOR_OK;
}

ThorStatus Filesystem::parseJson(const char *name, const std::vector<unsigned char> &contents,
                                 Json::Value &json) {
    const char *begin = reinterpret_cast<const char*>(contents.data());
    const char *end = begin+contents.size();
    std::string errors;
    bool ok = mJsonReader->parse(begin, end, &json, &errors);
    if (!ok) {
        LOGE("Json parse error from file %s: %s", name, errors.c_str());
        return TER_MISC_INVALID_FILE;
    }
    return THOR_OK;
}
