#ifndef PLAYLIST_H
#define PLAYLIST_H
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

class Playlist {
public:
    void BuildPlaylist(const std::wstring& currentFile);
    void BuildPlaylistFromDir(const std::wstring& dir);
    void Clear();
    void SetCurrentIndex(size_t index);
    void RemoveCurrent();
    
    const std::vector<std::wstring>& GetFiles() const { return m_files; }
    size_t GetCurrentIndex() const { return m_currentIndex; }

private:
    std::vector<std::wstring> m_files;
    size_t m_currentIndex = 0;
};
#endif