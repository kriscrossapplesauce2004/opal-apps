#pragma once
#include <QImage>
#include <QString>

// Loading and saving photos for edits. Blocking; call from worker threads only.
namespace ImageIO {

// Loads the full image with the EXIF orientation applied.
QImage loadOriented(const QString &path, QString *error = nullptr);
// Saves to dest in the format of its suffix. A JPEG keeps the EXIF block of original, with the orientation reset to normal.
bool save(const QImage &image, const QString &dest, const QString &original, QString *error = nullptr);
// Rotates by quarter turns clockwise. A JPEG with an orientation tag changes only that tag, so no quality is lost.
bool rotate(const QString &path, int quarterTurns, QString *error = nullptr);
// Writes the EXIF orientation of a JPEG in place. Returns false when the file has no orientation tag.
bool setJpegOrientation(const QString &path, int orientation);
int jpegOrientation(const QString &path);
// Returns "name-edited.ext", "name-edited-2.ext", ... next to the file, the first name that is free.
QString copyName(const QString &path);
// Composes an EXIF orientation with extra clockwise quarter turns.
int composeOrientation(int orientation, int quarterTurns);

}
