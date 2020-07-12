#pragma once

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include "IAssetRequest.h"
#include "TileContent.h"
#include "BoundingVolume.h"
#include "TileSelectionState.h"
#include "DoublyLinkedList.h"

namespace Cesium3DTiles {
    class Tileset;
    class TileContent;
    
    class CESIUM3DTILES_API Tile {
    public:
        enum class LoadState {
            /// <summary>
            /// Something went wrong while loading this tile.
            /// </summary>
            Failed = -1,

            /// <summary>
            /// The tile is not yet loaded at all, beyond the metadata in tileset.json.
            /// </summary>
            Unloaded = 0,

            /// <summary>
            /// The tile content is currently being loaded.
            /// </summary>
            ContentLoading = 1,

            /// <summary>
            /// The tile content has finished loading.
            /// </summary>
            ContentLoaded = 2,

            /// <summary>
            /// The tile's renderer resources are currently being prepared.
            /// </summary>
            RendererResourcesPreparing = 3,

            /// <summary>
            /// The tile's renderer resources are done being prepared and this
            /// tile is ready to render.
            /// </summary>
            RendererResourcesPrepared = 4
        };

        enum class Refine {
            Add = 0,
            Replace = 1
        };

        Tile();
        ~Tile();
        Tile(Tile& rhs) noexcept = delete;
        Tile(Tile&& rhs) noexcept;
        Tile& operator=(Tile&& rhs) noexcept;

        Tileset* getTileset() { return this->_pTileset; }
        const Tileset* getTileset() const { return this->_pTileset; }
        void setTileset(Tileset* pTileset) { this->_pTileset = pTileset; }

        Tile* getParent() { return this->_pParent; }
        const Tile* getParent() const { return this->_pParent; }
        void setParent(Tile* pParent) { this->_pParent = pParent; }

        gsl::span<const Tile> getChildren() const { return gsl::span<const Tile>(this->_children); }
        gsl::span<Tile> getChildren() { return gsl::span<Tile>(this->_children); }
        void createChildTiles(size_t count);

        const BoundingVolume& getBoundingVolume() const { return this->_boundingVolume; }
        void setBoundingVolume(const BoundingVolume& value) { this->_boundingVolume = value; }

        const std::optional<BoundingVolume>& getViewerRequestVolume() const { return this->_viewerRequestVolume; }
        void setViewerRequestVolume(const std::optional<BoundingVolume>& value) { this->_viewerRequestVolume = value; }

        double getGeometricError() const { return this->_geometricError; }
        void setGeometricError(double value) { this->_geometricError = value; }

        const std::optional<Refine>& getRefine() const { return this->_refine; }
        void setRefine(const std::optional<Refine>& value) { this->_refine = value; }

        /// <summary>
        /// Gets the transformation matrix for this tile. This matrix does _not_ need to be multiplied
        /// with the tile's parent's transform as this has already been done.
        /// </summary>
        const glm::dmat4x4& getTransform() const { return this->_transform; }
        void setTransform(const glm::dmat4x4& value) { this->_transform = value; }

        const std::optional<std::string>& getContentUri() const { return this->_contentUri; }
        void setContentUri(const std::optional<std::string>& value);

        const std::optional<BoundingVolume>& getContentBoundingVolume() const { return this->_contentBoundingVolume; }
        void setContentBoundingVolume(const std::optional<BoundingVolume>& value) { this->_contentBoundingVolume = value; }

        TileContent* getContent() { return this->_pContent.get(); }
        const TileContent* getContent() const { return this->_pContent.get(); }

        void* getRendererResources() { return this->_pRendererResources; }

        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }

        TileSelectionState& getLastSelectionState() { return this->_lastSelectionState; }
        const TileSelectionState& getLastSelectionState() const { return this->_lastSelectionState; }
        void setLastSelectionState(const TileSelectionState& newState) { this->_lastSelectionState = newState; }

        /**
         * Determines if this tile is currently renderable.
         */
        bool isRenderable() const { return this->getState() == LoadState::RendererResourcesPrepared; }

        void loadContent();
        void cancelLoadContent();

        /// <summary>
        /// Notifies the tile that its renderer resources have been prepared and optionally stores
        /// a pointer to those resources. This method is safe to call from any thread.
        /// </summary>
        /// <param name="pResource">The renderer resources as an opaque pointer.</param>
        void finishPrepareRendererResources(void* pResource = nullptr);

        DoublyLinkedListPointers<Tile> _loadedTilesLinks;

    protected:
        void setState(LoadState value);
        void contentResponseReceived(IAssetRequest* pRequest);

    private:
        // Position in bounding-volume hierarchy.
        Tileset* _pTileset;
        Tile* _pParent;
        std::vector<Tile> _children;

        // Properties from tileset.json.
        // These are immutable after the tile leaves TileState::Unloaded.
        BoundingVolume _boundingVolume;
        std::optional<BoundingVolume> _viewerRequestVolume;
        double _geometricError;
        std::optional<Refine> _refine;
        glm::dmat4x4 _transform;

        std::optional<std::string> _contentUri;
        std::optional<BoundingVolume> _contentBoundingVolume;

        // Load state and data.
        std::atomic<LoadState> _state;
        std::unique_ptr<IAssetRequest> _pContentRequest;
        std::unique_ptr<TileContent> _pContent;
        void* _pRendererResources;

        // Selection state
        TileSelectionState _lastSelectionState;

        friend class LoadedTilesList;
    };

}
