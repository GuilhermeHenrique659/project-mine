#include "graphics/texture_manager.hpp"

#include <utility>

#include "logging/logger.hpp"

namespace graphics {

TextureManager& TextureManager::instance() {
    static TextureManager manager;
    return manager;
}

void TextureManager::set_loader(std::unique_ptr<TextureLoader> loader) {
    unload_all();
    loader_ = std::move(loader);
}

void TextureManager::register_texture(const std::string& name, const std::string& path) {
    auto found = entries_.find(name);
    if (found == entries_.end()) {
        entries_[name] = Entry{path, std::nullopt, false};
        return;
    }
    if (found->second.texture && loader_) {
        loader_->unload(*found->second.texture);
    }
    found->second = Entry{path, std::nullopt, false};
}

const Texture* TextureManager::find(const std::string& name) {
    auto found = entries_.find(name);
    if (found == entries_.end() || !loader_) {
        return nullptr;
    }
    Entry& entry = found->second;
    if (entry.texture) {
        return &*entry.texture;
    }
    if (entry.failed) {
        return nullptr;
    }
    entry.texture = loader_->load(entry.path);
    if (!entry.texture) {
        entry.failed = true;
        logging::warning("Cannot load texture \"" + name + "\" from " + entry.path);
        return nullptr;
    }
    return &*entry.texture;
}

const Texture* TextureManager::get(const std::string& name) {
    if (const Texture* texture = find(name)) {
        return texture;
    }
    return fallback();
}

const Texture* TextureManager::fallback() {
    if (fallback_) {
        return &*fallback_;
    }
    if (!loader_ || fallback_failed_) {
        return nullptr;
    }
    fallback_ = loader_->create_placeholder();
    if (!fallback_) {
        fallback_failed_ = true;
        logging::warning("Cannot create the fallback texture");
        return nullptr;
    }
    return &*fallback_;
}

void TextureManager::unload_all() {
    if (fallback_ && loader_) {
        loader_->unload(*fallback_);
    }
    fallback_.reset();
    fallback_failed_ = false;
    for (auto& [name, entry] : entries_) {
        if (entry.texture && loader_) {
            loader_->unload(*entry.texture);
        }
        entry.texture.reset();
        entry.failed = false;
    }
}

void TextureManager::reset() {
    unload_all();
    entries_.clear();
    loader_.reset();
}

} // namespace graphics
