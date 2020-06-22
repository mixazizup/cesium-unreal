#include "Tileset.h"
#include "IAssetAccessor.h"
#include "IAssetResponse.h"
#include "Uri.h"
#pragma warning(push)
#pragma warning(disable: 4946)
#include "json.hpp"
#pragma warning(pop)

namespace Cesium3DTiles {

	Tileset::Tileset(const TilesetExternals& externals, const std::string& url) :
		_externals(externals),
		_views(),
		_url(url),
		_ionAssetID(),
		_ionAccessToken(),
		_pTilesetRequest(),
		_tiles(),
		_pRootTile()
	{
		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	Tileset::Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken) :
		_externals(externals),
		_views(),
		_url(),
		_ionAssetID(ionAssetID),
		_ionAccessToken(ionAccessToken),
		_pTilesetRequest(),
		_tiles(),
		_pRootTile()
	{
		std::string url = "https://api.cesium.com/v1/assets/" + std::to_string(ionAssetID) + "/endpoint";
		if (ionAccessToken.size() > 0)
		{
			url += "?access_token=" + ionAccessToken;
		}

		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::ionResponseReceived, this, std::placeholders::_1));
	}

	TilesetView& Tileset::createView(const std::string& name)
	{
		TilesetView* p = new TilesetView(*this, name);
		this->_views.push_back(std::move(std::unique_ptr<TilesetView>(p)));
		return *p;
	}

	void Tileset::destroyView(TilesetView& view)
	{
		this->_views.erase(
			std::remove_if(
				this->_views.begin(),
				this->_views.end(),
				[&view](const std::unique_ptr<TilesetView>& candidate) {
					return candidate.get() == &view;
				}
			),
			this->_views.end()
					);
	}

	void Tileset::ionResponseReceived(IAssetRequest* pRequest) {
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

		using nlohmann::json;
		json ionResponse = json::parse(data.begin(), data.end());

		std::string url = ionResponse.value<std::string>("url", "");
		std::string accessToken = ionResponse.value<std::string>("accessToken", "");
		std::string urlWithToken = Uri::addQuery(url, "access_token", accessToken);

		// When we assign _pTilesetRequest, the previous request and response
		// that we're currently handling may immediately be deleted.
		pRequest = nullptr;
		pResponse = nullptr;
		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(urlWithToken);
		this->_pTilesetRequest->bind(std::bind(&Tileset::tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	void Tileset::tilesetJsonResponseReceived(IAssetRequest* pRequest) {
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

		using nlohmann::json;
		json tileset = json::parse(data.begin(), data.end());

		std::string baseUrl = pRequest->url();

		pRequest = nullptr;
		pResponse = nullptr;
		this->_pTilesetRequest.reset();

		json& rootJson = tileset["root"];

		this->_tiles.emplace_back(*this);
		VectorReference<Tile> rootTile(this->_tiles, this->_tiles.size() - 1);

		this->createTile(rootTile, rootJson, baseUrl);
		this->_pRootTile = rootTile;
	}

	void Tileset::createTile(VectorReference<Tile>& tile, const nlohmann::json& tileJson, const std::string& baseUrl) {
		using nlohmann::json;

		if (!tileJson.is_object())
		{
			return;
		}

		const bool leavesOnly = true;

		json::const_iterator contentIt = tileJson.find("content");
		json::const_iterator childrenIt = tileJson.find("children");

		if (contentIt != tileJson.end())
		{
			const std::string& uri = contentIt->value<std::string>("uri", contentIt->value<std::string>("url", ""));
			const std::string fullUri = Uri::resolve(baseUrl, uri, true);
			tile->setContentUri(fullUri);
		}

		if (childrenIt != tileJson.end())
		{
			const json& childrenJson = *childrenIt;
			if (!childrenJson.is_array())
			{
				return;
			}

			// Allocate children contiguously and efficiently from a vector.
			size_t firstChild = this->_tiles.size();

			for (size_t i = 0; i < childrenJson.size(); ++i) {
				this->_tiles.emplace_back(*this, tile);
			}

			size_t afterLastChild = this->_tiles.size();

			for (size_t i = 0; i < childrenJson.size(); ++i) {
				const json& childJson = childrenJson[i];
				VectorReference<Tile> child(this->_tiles, firstChild + i);
				this->createTile(child, childJson, baseUrl);
			}

			VectorRange<Tile> childTiles(this->_tiles, firstChild, afterLastChild);
			tile->setChildren(childTiles);
		}
	}

}
