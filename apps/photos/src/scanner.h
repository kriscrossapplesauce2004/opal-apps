#pragma once
#include "photo.h"

#include <QHash>
#include <QList>
#include <QStringList>
#include <atomic>
#include <functional>

// Walks photo folders on a worker thread. Pure functions: no Qt objects, no GUI access.
namespace Scanner {

struct Options
{
    QStringList roots;
    QStringList excluded;
    // Skips folders that hold programs, games or source code, so their textures stay out of the timeline.
    bool skipSoftware = true;
    bool videos = true;
};

struct Result
{
    QList<Photo> photos;
    // Every folder that was walked, for the file system watcher.
    QStringList folders;
    // Folders skipped because they look like software.
    QStringList skipped;
};

// The cache maps a path to its last record; unchanged files (same mtime and size) are not probed again.
Result scan(const Options &options, const QHash<QString, Photo> &cache, const std::atomic_bool *cancel = nullptr,
            const std::function<void(int)> &progress = {});

// Probes one file: size, orientation, capture date, camera. Returns false when it is not a readable image or video.
bool probe(const QString &path, Photo &out);

// Captions live in the user.xdg.comment extended attribute, which Dolphin and Nautilus also read.
QString readCaption(const QString &path);
bool writeCaption(const QString &path, const QString &caption);

bool isImageName(const QString &name);
bool isVideoName(const QString &name);
bool isScreenshot(const QString &path);
// True when the folder holds a marker of software (an executable, a library, a .git folder, a build file, .nomedia).
bool hasSoftwareMarker(const QString &folder);
// Looks a few levels down for a software marker. Used for the direct children of a root.
bool looksLikeSoftware(const QString &folder, int depth = 3);

}
