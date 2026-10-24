# Constants generated from the source asset manifests (bbassets ids):
# symbolic names of sprite images. The headers are committed; the test
# assets.ids checks that they match assets/source.
add_library(bb_asset_ids INTERFACE)
add_library(bb::asset_ids ALIAS bb_asset_ids)
target_include_directories(bb_asset_ids INTERFACE include)
