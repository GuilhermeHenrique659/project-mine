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

void TextureManager::release(Entry& entry) {
    if (entry.texture && loader_) {
        loader_->unload(*entry.texture);
    }
    entry.texture.reset();
    entry.failed = false;
}

TextureId TextureManager::register_texture(const std::string& name, const std::string& path) {
    const auto found = ids_.find(name);
    if (found == ids_.end()) {
        const auto id = static_cast<TextureId>(entries_.size());
        entries_.push_back(Entry{path, std::nullopt, false});
        ids_.emplace(name, id);
        return id;
    }
    Entry& entry = entries_[static_cast<std::size_t>(found->second)];
    release(entry);
    entry.path = path;
    return found->second;
}

const Texture* TextureManager::find(const std::string& name) {
    const auto found = ids_.find(name);
    if (found == ids_.end()) {
        return nullptr;
    }
    return find(found->second);
}

const Texture* TextureManager::find(TextureId id) {
    const auto index = static_cast<std::size_t>(id);
    if (index >= entries_.size() || !loader_) {
        return nullptr;
    }
    Entry& entry = entries_[index];
    if (entry.texture) {
        return &*entry.texture;
    }
    if (entry.failed) {
        return nullptr;
    }
    entry.texture = loader_->load(entry.path);
    if (!entry.texture) {
        entry.failed = true;
        logging::warning("Cannot load texture from " + entry.path);
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
    for (Entry& entry : entries_) {
        release(entry);
    }
}

void TextureManager::reset() {
    unload_all();
    entries_.clear();
    ids_.clear();
    loader_.reset();
}

} // namespace graphics
