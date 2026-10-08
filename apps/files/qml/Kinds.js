.pragma library
// Material Symbols glyph and tone for each file kind the models report.
var glyph = {folder: "folder", image: "image", video: "movie", audio: "music_note", pdf: "picture_as_pdf", archive: "folder_zip", text: "description", code: "code", app: "deployed_code", other: "draft"};
var filters = [{id: "image", name: "Images", icon: "image"}, {id: "video", name: "Videos", icon: "movie"}, {id: "audio", name: "Audio", icon: "music_note"}, {id: "folder", name: "Folders", icon: "folder"}, {id: "pdf", name: "Documents", icon: "description", also: ["text"]}, {id: "archive", name: "Archives", icon: "folder_zip"}];
function tone(theme, kind) {
    if (kind === "folder") return theme.primary;
    if (kind === "pdf") return theme.error;
    if (kind === "archive" || kind === "video") return theme.tertiary;
    return theme.secondary;
}
