#include "src/game/envfx_snow.h"

const GeoLayout camera_geo[] = {
	GEO_NODE_START(),
	GEO_OPEN_NODE(),
		GEO_WIREFRAME(),
		GEO_OPEN_NODE(),
			GEO_DISPLAY_LIST(LAYER_OPAQUE, camera_camera_Root_mesh_layer_1_mesh_mesh_layer_1),
			GEO_ASM(0, geo_camera_scale),
			GEO_SCALE(0x00, 65536),
			GEO_OPEN_NODE(),
				GEO_DISPLAY_LIST(LAYER_OPAQUE, camera_camera_Icosphere_001_mesh_layer_1_mesh_mesh_layer_1),
			GEO_CLOSE_NODE(),
		GEO_CLOSE_NODE(),
	GEO_CLOSE_NODE(),
	GEO_END(),
};
