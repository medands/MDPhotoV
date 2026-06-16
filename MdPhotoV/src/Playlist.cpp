#include "Playlist.h"
#include <algorithm>

void Playlist::BuildPlaylist(const std::wstring& currentFile) {
    m_files.clear();
    m_currentIndex = 0;
    fs::path p(currentFile);
    if (!fs::exists(p)) return;

    fs::path dir = p.parent_path();
    std::vector<std::wstring> exts = { L".jpg", L".jpeg", L".png", L".bmp", L".avif", L".webp", L".gif", L".tiff" };

    try {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::wstring ext = entry.path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
                    m_files.push_back(entry.path().wstring());
                }
            }
        }
    } 
	catch (...) { return; }

    std::sort(m_files.begin(), m_files.end());

    for (size_t i = 0; i < m_files.size(); ++i) {
        if (m_files[i] == p.wstring()) {
            m_currentIndex = i;
            break;
        }
    }
}

void Playlist::BuildPlaylistFromDir(const std::wstring& dir) {
    m_files.clear();
    m_currentIndex = 0;
    std::vector<std::wstring> exts = { L".jpg", L".jpeg", L".png", L".bmp", L".avif", L".webp", L".gif", L".tiff" };

    try {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::wstring ext = entry.path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
                    m_files.push_back(entry.path().wstring());
                }
            }
        }
    } 
	catch (...) { return; }

    std::sort(m_files.begin(), m_files.end());
}

void Playlist::Clear() {
    m_files.clear();
    m_currentIndex = 0;
}

void Playlist::SetCurrentIndex(size_t index){
    if (index < m_files.size()) {
        m_currentIndex = index;
    }
}

void Playlist::RemoveCurrent() {
    if (m_files.empty()) return;
    if (m_currentIndex < m_files.size()) {
        m_files.erase(m_files.begin() + m_currentIndex);
        if (m_currentIndex >= m_files.size() && m_files.size() > 0) {
            m_currentIndex = m_files.size() - 1;
        }
    }
}