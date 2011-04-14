
#include "precompiled.h"

#include "FBCopyFile.h"

#include "../filesystem/input_stream.h"
#include "../filesystem/file_package_manager.h"

#include <fstream>
#include <string>

namespace util
{

void FBCopyFile::copyFile(const std::string &from, const std::string &to)
{
	// std::fstream	in( from.c_str(), std::ios::in );
	std::fstream	out( to.c_str(), std::ios::out );
	frozenbyte::filesystem::InputStream in = frozenbyte::filesystem::FilePackageManager::getInstance().getFile( from );

	
	std::string temp;
	temp.resize( in.getSize() );
	in.read( (unsigned char *)&temp[0], in.getSize() );
	out << temp << std::endl;

	/*if( in.is_open() )
	{*/
		/*while( in.isEof() )
		{
			std::string temp;
			std::getline( in, ( temp ) );
			out << temp << std::endl; 
		}
	// }*/

	// in.close();
	out.close();	
}

}
