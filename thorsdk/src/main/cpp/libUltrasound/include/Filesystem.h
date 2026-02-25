//
// Copyright 2020 EchoNous Inc.
//
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <json/reader.h>
#include <json/value.h>
#include <ThorError.h>

struct AAssetManager;

class Filesystem {
public:
    Filesystem();

    /**
     * Initialize the Filesystem with Android asset manager and application path.
     * @param assetManager Android asset manager, may be NULL
     * @param appPath Application path root, MUST NOT be NULL
     * @return Status of initialization
     */
    ThorStatus init(AAssetManager *assetManager, const char *appPath);

    /**
     * Read a file from assets (assets may either be in Android asset manager or overridden on the
     * filesystem).
     * @param name Path to asset, relative to assets root
     * @param buffer Buffer to receive asset data
     * @return Status of read operation
     */
    ThorStatus readAsset(const char *name, std::vector<unsigned char> &buffer);

    /**
     * Read a file from cache. A file in the cache MAY NOT EXIST, so the caller MUST handle a missing
     * file without error.
     * @param name Path to file, relative to cache root
     * @param buffer Buffer to receive file data
     * @return Status of read operation
     */
    ThorStatus readCache(const char *name, std::vector<unsigned char> &buffer);

    /**
     * Read an asset and parse as json.
     * @param name Path to asset, relative to assets root
     * @param json Value to read into
     * @return Status of read and parse operations
     */
    ThorStatus readAssetJson(const char *name, Json::Value &json);

    /**
     * Read cache data and parse as json
     * @param name Path to asset, relative to assets root
     * @param json Value to read into
     * @return Status of read and parse operations
     */
    ThorStatus readCacheJson(const char *name, Json::Value &json);

    /**
     * Write a file to the cache.
     * @param name Path to file, relative to cache root
     * @param buffer Contents to write
     * @return Status of write operation
     */
    ThorStatus writeCache(const char *name, const std::vector<unsigned char> &buffer);

    /**
     * List the contents of a directory in assets. This is the combined contents of the directory
     * in the Android asset manager, and the filesystem override. Only regular files are listed
     * (not subdirectories).
     *
     * @param dir Path to directory, relative to assets root
     * @param entries List of entries. Each entry is a filename/directory name ONLY, and is relative
     *                to the parent directory (NOT relative to assets root)
     * @return Status of read directory operation
     */
    ThorStatus listAssetDirectory(const char *dir, std::vector<std::string> &entries);

    /**
     * List the contents of a directory in the cache. Only regular files are listed, not
     * subdirectories.
     *
     * @param dir Path to directory, relative to cache root
     * @param entries List of entries. Each entry is a filename/directory name ONLY, and is relative
     *                to the parent directory (NOT relative to cache root)
     * @return Status of read directory operation
     */
    ThorStatus listCacheDirectory(const char *dir, std::vector<std::string> &entries);

    /**
     * Get the underlying Android Asset Manager
     * @return Android Asset Manager, or NULL if not set up.
     */
    AAssetManager *getAAssetManager() const;

    /**
     * Get the path to the assets directory (on the filesystem). This path always ends with the '/'
     * separator character.
     * @return Path to assets directory
     */
    const std::string& getAssetDir() const;

    /**
     * Get the path to the cache directory (on the filesystem). This path always ends with the '/'
     * separator character.
     * @return Path to cache directory
     */
    const std::string& getCacheDir() const;
private:

    ThorStatus readFromAssets(const char *name, std::vector<unsigned char> &buffer);
    ThorStatus readFromFilesystem(const char *name, std::vector<unsigned char> &buffer);

    /**
     * Make a (single) directory on the filesystem. The parent directory must already exist
     * @param dir Directory to create
     * @param existOk If true, then creating an existing directory is not an error
     * @return Create directory status
     */
    ThorStatus makeDirectory(const char *dir, bool existOk = true);

    /**
     * Ensures that a path exists on the filesystem (like `mkdir -p`)
     * @param path Full path to create
     * @return Error if creating any directory in path fails, THOR_OK otherwise
     */
    ThorStatus ensurePath(const char *path);

    ThorStatus listDirectoryAssetManager(const char *path, std::vector<std::string> &entries);
    ThorStatus listDirectoryFilesystem(const char *path, std::vector<std::string> &entries);

    ThorStatus parseJson(const char *name, const std::vector<unsigned char> &contents, Json::Value &json);

    AAssetManager *mAssetManager;
    std::string    mAssetsDir;
    std::string    mCacheDir;
    std::unique_ptr<Json::CharReader> mJsonReader;
};