/*******************************************************************************
#      ____               __          __  _      _____ _       _               #
#     / __ \              \ \        / / | |    / ____| |     | |              #
#    | |  | |_ __   ___ _ __ \  /\  / /__| |__ | |  __| | ___ | |__   ___      #
#    | |  | | '_ \ / _ \ '_ \ \/  \/ / _ \ '_ \| | |_ | |/ _ \| '_ \ / _ \     #
#    | |__| | |_) |  __/ | | \  /\  /  __/ |_) | |__| | | (_) | |_) |  __/     #
#     \____/| .__/ \___|_| |_|\/  \/ \___|_.__/ \_____|_|\___/|_.__/ \___|     #
#           | |                                                                #
#           |_|                                                                #
#                                                                              #
#                                (c) 2011 by                                   #
#           University of Applied Sciences Northwestern Switzerland            #
#                     Institute of Geomatics Engineering                       #
#                           robert.wueest@fhnw.ch                              #
********************************************************************************
*     Licensed under MIT License. Read the file LICENSE for more information   *
*******************************************************************************/
// This is the triangulate version without mpi intended for regular 
// workstations. Multi cores are supported (OpenMP) and highly recommended.
//------------------------------------------------------------------------------
// Some code adapted from: generate_tiles.py
// Found at: http://trac.openstreetmap.org/browser/applications/rendering/mapnik
//------------------------------------------------------------------------------
#include <mapnik/map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/filter_factory.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/envelope.hpp>
#include <mapnik/proj_transform.hpp>
#include <iostream>
#include <boost/filesystem.hpp>
#include "render_tile.h"
#include <string/FilenameUtils.h>
#include <string/StringUtils.h>
#include <io/FileSystem.h>
#include <math/mathutils.h>

//------------------------------------------------------------------------------------
int main ( int argc , char** argv)
{    
    if (argc != 3)
    {
        std::cout << "needs at least two argument";
        return EXIT_SUCCESS;
    }
    
    using namespace mapnik;
    try {
        std::cout << " generating map ... \n";
        std::string mapnik_dir(argv[1]);
		std::string map_file(argv[2]);
		std::string map_uri = "image.png";
		std::string output_path = "D:/data/test/";
		int minZoom = 1;
		int maxZoom = 12;
		bool tsmScheme = false;
		GoogleProjection gProj = GoogleProjection(maxZoom); // maxlevel 12
		//projection merc = projection("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over");
		//projection longlat = projection("+proj=latlong +datum=WGS84");

		// Boundaries
		// CH Bounds  double bounds[4] = {5.955870,46.818020,10.492030,47.808380};
		double bounds[4] = {-180,-90,180,90};
		double dummy = 0.0;
	
		//proj_transform transform = proj_transform(longlat,merc);

		int x = 1;
		int y = 1;
		//int z = 2;

		
		
#ifdef _DEBUG
		std::string plugin_path = mapnik_dir + "/input/debug/";
#else
		std::string plugin_path = mapnik_dir + "/input/release/";	
#endif
		datasource_cache::instance()->register_datasources(plugin_path.c_str()); 
		std::string font_dir = mapnik_dir + "/fonts/dejavu-fonts-ttf-2.30/ttf/";
        std::cout << " looking for DejaVuSans fonts in... " << font_dir << "\n";
		if (boost::filesystem3::exists( font_dir ) )
		{
			boost::filesystem3::directory_iterator end_itr; // default construction yields past-the-end
			for ( boost::filesystem3::directory_iterator itr( font_dir );
				itr != end_itr;
				++itr )
			{
				if (!boost::filesystem3::is_directory(itr->status()) )
				{
					freetype_engine::register_font(itr->path().string());
				}
			}
		}
		// Generate map container
		Map m(tilesize,tilesize);
        m.set_background(color_factory::from_string("white"));
		load_map(m,map_file);
		projection mapnikProj = projection(m.srs());

		if(!FileSystem::DirExists(output_path))
			FileSystem::makedir(output_path);

		for(int z = minZoom; z < maxZoom + 1; z++)
		{
			ituple px0 = gProj.geoCoord2Pixel(dtuple(bounds[0], bounds[3]),z);
			ituple px1 = gProj.geoCoord2Pixel(dtuple(bounds[2], bounds[1]),z);

			// check if we have directories in place
			std::string szoom = StringUtils::IntegerToString(z, 10);
			if(!FileSystem::DirExists(output_path + szoom))
				FileSystem::makedir(output_path + szoom);

			for(int x = int(px0.a/256.0); x <= int(px1.a/256.0) +1; x++)
			{
				// Validate x co-ordinate
				if((x < 0) || (x >= math::Pow2(z)))
					continue;
				// check if we have directories in place
				std::string str_x = StringUtils::IntegerToString(x,10);
				if(!FileSystem::DirExists(output_path + szoom + "/" + str_x))
					FileSystem::makedir(output_path + szoom + "/" + str_x);
				for(int y = int(px0.b/256.0); y <= int(px1.b/256.0)+1; y++)
				{
					// Validate x co-ordinate
					if((y < 0) || (y >= math::Pow2(z)))
						continue;
					// flip y to match OSGEO TMS spec
					std::string str_y;
					if(tsmScheme)
						str_y = StringUtils::IntegerToString(math::Pow2(z-1) - y,10);
					else
						str_y = StringUtils::IntegerToString(y,10);

					std::string tile_uri = output_path + szoom + '/' + str_x + '/' + str_y + ".png";
					// Submit tile to be rendered
					_renderTile(tile_uri,m,x,y,z,gProj,mapnikProj);
				}
			}
		}
		// Submit tile to be rendered into the queue
		/*			t = (name, tile_uri, x, y, z)
                try:
                    queue.put(t)
                except KeyboardInterrupt:
                    raise SystemExit("Ctrl-c detected, exiting...")*/
    }
    catch ( const mapnik::config_error & ex )
    {
        std::cerr << "### Configuration error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch ( const std::exception & ex )
    {
        std::cerr << "### std::exception: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch ( ... )
    {
        std::cerr << "### Unknown exception." << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
