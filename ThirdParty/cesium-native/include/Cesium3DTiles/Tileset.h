#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <atomic>
#include "TilesetExternals.h"
#include "Tile.h"
#include "IAssetRequest.h"
#include "ViewUpdateResult.h"
#include "Camera.h"
#include "Json.h"

/**
 * Additional options for configuring a \ref Tileset.
 * 
 */
struct TilesetOptions {
    /**
     * The maximum number of pixels of error when rendering this tileset.
     * This is used to select an appropriate level-of-detail.
     */
    double maximumScreenSpaceError = 16.0;

    /**
     * The maximum number of tiles that may simultaneously be in the process
     * of loading.
     */
    uint32_t maximumSimultaneousTileLoads = 10;

    /**
     * Indicates whether the ancestors of rendered tiles should be preloaded.
     * Setting this to true optimizes the zoom-out experience and provides more detail in
     * newly-exposed areas when panning. The down side is that it requires loading more tiles.
     */
    bool preloadAncestors = true;

    /**
     * Indicates whether the siblings of rendered tiles should be preloaded.
     * Setting this to true causes tiles with the same parent as a rendered tile to be loaded, even
     * if they are culled. Setting this to true may provide a better panning experience at the
     * cost of loading more tiles.
     */
    bool preloadSiblings = false;

    /**
     * The number of loading descendant tiles that is considered "too many".
     * If a tile has too many loading descendants, that tile will be loaded and rendered before any of
     * its descendants are loaded and rendered. This means more feedback for the user that something
     * is happening at the cost of a longer overall load time. Setting this to 0 will cause each
     * tile level to be loaded successively, significantly increasing load time. Setting it to a large
     * number (e.g. 1000) will minimize the number of tiles that are loaded but tend to make
     * detail appear all at once after a long wait.
     */
    uint32_t loadingDescendantLimit = 20;
};

namespace Cesium3DTiles {

    /**
     * A {@link https://github.com/CesiumGS/3d-tiles/tree/master/specification|3D Tiles tileset},
     * used for streaming massive heterogeneous 3D geospatial datasets.
     */
    class CESIUM3DTILES_API Tileset {
    public:
        /**
         * Constructs a new instance with a given `tileset.json` URL.
         * @param externals The external interfaces to use.
         * @param url The URL of the `tileset.json`.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, const std::string& url, const TilesetOptions& options = TilesetOptions());

        /**
         * Constructs a new instance with the given asset ID on <a href="https://cesium.com/ion/">Cesium ion</a>.
         * @param externals The external interfaces to use.
         * @param ionAssetID The ID of the Cesium ion asset to use.
         * @param ionAccessToken The Cesium ion access token authorizing access to the asset.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken, const TilesetOptions& options = TilesetOptions());

        /**
         * Gets the URL that was used to construct this tileset. If the tileset references a Cesium ion asset,
         * this property will not have a value.
         */
        std::optional<std::string> getUrl() const { return this->_url; }

        /**
         * Gets the Cesium ion asset ID of this tileset. If the tileset references a URL, this property
         * will not have a value.
         */
        std::optional<uint32_t> getIonAssetID() const { return this->_ionAssetID; }

        /**
         * Gets the Cesium ion access token to use to access this tileset. If the tileset references a URL, this
         * property will not have a value.
         */
        std::optional<std::string> getIonAccessToken() const { return this->_ionAccessToken; }

        /**
         * Gets the external interfaces used by this tileset.
         */
        TilesetExternals& externals() { return this->_externals; }
        const TilesetExternals& getExternals() const { return this->_externals; }

        /**
         * Gets the root tile of this tileset, or nullptr if there is currently no root tile.
         */
        Tile* getRootTile() { return this->_pRootTile.data(); }
        const Tile* getRootTile() const { return this->_pRootTile.data(); }

        /**
         * Updates this view, returning the set of tiles to render in this view.
         * @param camera The updated camera.
         * @returns The set of tiles to render in the updated camera view. This value is only valid until
         *          the next call to <see cref="Cesium3DTilesetView.update" /> or until the view is
         *          destroyed, whichever comes first.
         */
        const ViewUpdateResult& updateView(const Camera& camera);

        /**
         * Notifies the tileset that the given tile has finished loading and is ready to render.
         * This method may be called from any thread.
         */
        void notifyTileDoneLoading(Tile* pTile);

    private:
        struct TraversalDetails {
            /**
             * True if all selected (i.e. not culled or refined) tiles in this tile's subtree
             * are renderable. If the subtree is renderable, we'll render it; no drama.
             */
            bool allAreRenderable = true;

            /**
             * True if any tiles in this tile's subtree were rendered last frame. If any
             * were, we must render the subtree rather than this tile, because rendering
             * this tile would cause detail to vanish that was visible last frame, and
             * that's no good.
             */
            bool anyWereRenderedLastFrame = false;

            /**
             * Counts the number of selected tiles in this tile's subtree that are
             * not yet ready to be rendered because they need more loading. Note that
             * this value will _not_ necessarily be zero when
             * /ref TraversalDetails::allAreRenderable is true, for subtle reasons.
             * When /ref TraversalDetails::allAreRenderable and
             * /ref TraversalDetails::anyWereRenderedLastFrame are both false, we
             * will render this tile instead of any tiles in its subtree and
             * the `allAreRenderable` value for this tile will reflect only whether _this_
             * tile is renderable. The `notYetRenderableCount` value, however, will still
             * reflect the total number of tiles that we are waiting on, including the
             * ones that we're not rendering. `notYetRenderableCount` is only reset
             * when a subtree is removed from the render queue because the
             * `notYetRenderableCount` exceeds the
             * /ref TilesetOptions::loadingDescendantLimit.
             */
            uint32_t notYetRenderableCount = 0;
        };

        void _ionResponseReceived(IAssetRequest* pRequest);
        void _tilesetJsonResponseReceived(IAssetRequest* pRequest);
        void _createTile(VectorReference<Tile>& tile, const nlohmann::json& tileJson, const std::string& baseUrl);
        TraversalDetails _visitTile(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, Tile& tile, ViewUpdateResult& result);
        TraversalDetails _visitTileIfVisible(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, Tile& tile, ViewUpdateResult& result);
        TraversalDetails _visitVisibleChildrenNearToFar(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, Tile& tile, ViewUpdateResult& result);
        void _processLoadQueue();

        TilesetExternals _externals;

        std::optional<std::string> _url;
        std::optional<uint32_t> _ionAssetID;
        std::optional<std::string> _ionAccessToken;

        TilesetOptions _options;

        std::unique_ptr<IAssetRequest> _pTilesetRequest;

        std::vector<Tile> _tiles;
        VectorReference<Tile> _pRootTile;

        uint32_t _previousFrameNumber;
        ViewUpdateResult _updateResult;

        std::vector<Tile*> _loadQueueHigh;
        std::vector<Tile*> _loadQueueMedium;
        std::vector<Tile*> _loadQueueLow;
        std::atomic<uint32_t> _loadsInProgress;

        Tileset(const Tileset& rhs) = delete;
        Tileset& operator=(const Tileset& rhs) = delete;
    };

} // namespace Cesium::ThreeDTiles