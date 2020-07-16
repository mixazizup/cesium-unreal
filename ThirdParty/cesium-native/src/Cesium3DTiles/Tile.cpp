#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/TileContentFactory.h"

namespace Cesium3DTiles {

    Tile::Tile() :
        _loadedTilesLinks(),
        _pTileset(nullptr),
        _pParent(nullptr),
        _children(),
        _boundingVolume(BoundingBox(glm::dvec3(), glm::dmat4())),
        _viewerRequestVolume(),
        _geometricError(0.0),
        _refine(),
        _transform(1.0),
        _contentUri(),
        _contentBoundingVolume(),
        _state(LoadState::Unloaded),
        _pContentRequest(nullptr),
        _pContent(nullptr),
        _pRendererResources(nullptr),
        _lastSelectionState()
    {
    }

    Tile::~Tile() {
    }

    Tile::Tile(Tile&& rhs) noexcept :
        _loadedTilesLinks(std::move(rhs._loadedTilesLinks)),
        _pTileset(rhs._pTileset),
        _pParent(rhs._pParent),
        _children(std::move(rhs._children)),
        _boundingVolume(rhs._boundingVolume),
        _viewerRequestVolume(rhs._viewerRequestVolume),
        _geometricError(rhs._geometricError),
        _refine(rhs._refine),
        _transform(rhs._transform),
        _contentUri(rhs._contentUri),
        _contentBoundingVolume(rhs._contentBoundingVolume),
        _state(rhs.getState()),
        _pContentRequest(std::move(rhs._pContentRequest)),
        _pContent(std::move(rhs._pContent)),
        _pRendererResources(rhs._pRendererResources),
        _lastSelectionState(rhs._lastSelectionState)
    {
    }

    Tile& Tile::operator=(Tile&& rhs) noexcept {
        if (this != &rhs) {
            this->_loadedTilesLinks = std::move(rhs._loadedTilesLinks);
            this->_pTileset = rhs._pTileset;
            this->_pParent = rhs._pParent;
            this->_children = std::move(rhs._children);
            this->_boundingVolume = rhs._boundingVolume;
            this->_viewerRequestVolume = rhs._viewerRequestVolume;
            this->_geometricError = rhs._geometricError;
            this->_refine = rhs._refine;
            this->_transform = rhs._transform;
            this->_contentUri = rhs._contentUri;
            this->_contentBoundingVolume = rhs._contentBoundingVolume;
            this->setState(rhs.getState());
            this->_pContentRequest = std::move(rhs._pContentRequest);
            this->_pContent = std::move(rhs._pContent);
            this->_pRendererResources = rhs._pRendererResources;
            this->_lastSelectionState = rhs._lastSelectionState;
        }

        return *this;
    }

    void Tile::createChildTiles(size_t count) {
        if (this->_children.size() > 0) {
            throw std::runtime_error("Children already created.");
        }
        this->_children.resize(count);
    }

    void Tile::setContentUri(const std::optional<std::string>& value)
    {
        this->_contentUri = value;
    }

    void Tile::loadContent()
    {
        if (this->getState() != LoadState::Unloaded) {
            return;
        }

        if (!this->getContentUri().has_value()) {
            // TODO: should we let the renderer do some preparation even if there's no content?
            this->setState(LoadState::RendererResourcesPrepared);
            this->_pTileset->notifyTileDoneLoading(this);
            return;
        }

        IAssetAccessor* pAssetAccessor = this->_pTileset->getExternals().pAssetAccessor;
        this->_pContentRequest = pAssetAccessor->requestAsset(*this->_contentUri);
        this->_pContentRequest->bind(std::bind(&Tile::contentResponseReceived, this, std::placeholders::_1));

        this->setState(LoadState::ContentLoading);
    }

    void Tile::unloadContent() {
        const TilesetExternals& externals = this->_pTileset->getExternals();
        if (externals.pPrepareRendererResources) {
            externals.pPrepareRendererResources->free(*this, this->_pRendererResources);
        }

        this->_pRendererResources = nullptr;
        this->_pContent.reset();
        this->setState(LoadState::Unloaded);
    }

    void Tile::cancelLoadContent() {
        if (this->_pContentRequest) {
            this->_pContentRequest->cancel();
            this->_pContentRequest.release();

            if (this->getState() == LoadState::ContentLoading) {
                this->setState(LoadState::Unloaded);
            }
        }
    }

    void Tile::setState(LoadState value) {
        this->_state.store(value, std::memory_order::memory_order_release);
    }

    void Tile::contentResponseReceived(IAssetRequest* pRequest) {
        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            return;
        }

        gsl::span<const uint8_t> data = pResponse->data();

        const TilesetExternals& externals = this->_pTileset->getExternals();

        externals.pTaskProcessor->startTask([data, this]() {
            std::unique_ptr<TileContent> pContent = TileContentFactory::createContent(*this, data);
            if (!pContent) {
                // Try to load this content as an external tileset.json.
                using nlohmann::json;
                json tilesetJson = json::parse(data.begin(), data.end());
                std::vector<Tile> externalRoot(1);
                externalRoot[0].setParent(this);
                this->getTileset()->loadTilesFromJson(externalRoot[0], tilesetJson, this->_pContentRequest->url());

                // TODO: this is a race condition, because there is no guarantee that a move of a vector is atomic.
                this->_children = std::move(externalRoot);

                // Bump up the geometric error so we always refine past this no-content tile.
                this->setGeometricError(9999999999.0);
    
                this->finishPrepareRendererResources(nullptr);
            } else {
                this->_pContent = std::move(pContent);
                this->setState(LoadState::ContentLoaded);

                const TilesetExternals& externals = this->_pTileset->getExternals();
                if (externals.pPrepareRendererResources) {
                    this->setState(LoadState::RendererResourcesPreparing);
                    externals.pPrepareRendererResources->prepare(*this);
                }
                else {
                    this->finishPrepareRendererResources(nullptr);
                }
            }

            // TODO
            //this->_pContentRequest.reset();
        });
    }

    void Tile::finishPrepareRendererResources(void* pResource) {
        this->_pRendererResources = pResource;
        this->setState(LoadState::RendererResourcesPrepared);
        this->_pTileset->notifyTileDoneLoading(this);
    }

}
