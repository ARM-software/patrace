set (SRC_IMAGE
	image.cpp
	image_io.cpp
	image_pnm.cpp
	image_png.cpp
	image_ktx.cpp
	image_astc.cpp
	image_compression.cpp
	image_compression_btc.cpp
	image_compression_etc.cpp
	image_compression_astc.cpp
) 

add_library (common_image STATIC
    ${SRC_IMAGE}
)
