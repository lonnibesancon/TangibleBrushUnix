#include "loader_obj.h"

#include "rendering/mesh.h"
#include "util/file.h"

#include <sstream>
#include <streambuf>
#include <climits>

namespace
{
	void parseV(std::istream& stream, MeshData& data)
	{
		std::string str;
		stream >> str;

		// std::cout << "parseV " << str << '\n';

		if (str == "v") {
			float x, y, z;
			stream >> x >> y >> z;
			// std::cout << x << ' ' << y << ' ' << z << '\n';
			data.vertices.push_back(Vector3(x, y, z));
		} else if (str == "vt") {
			float u, v;
			stream >> u >> v;
			// std::cout << u << ' ' << v << '\n';
			data.texCoords.push_back(Vector2(u, 1.0f - v));
		} else if (str == "vn") {
			float x, y, z;
			stream >> x >> y >> z;
			// std::cout << x << ' ' << y << ' ' << z << '\n';
			data.normals.push_back(Vector3(x, y, z));
		}
	}

	void parseF(std::istream& stream, MeshData& data, const std::string& fileName)
	{
		stream.ignore(1, '\n'); // skip 'f'

		// std::cout << "parseF" << '\n';

		for (int i = 0; i < 3; ++i) {
			std::string str;
			stream >> str;

			const std::vector<std::string> indices = Utility::split(str, '/');

			if (indices.empty())
				throw std::runtime_error("Invalid face: \"" + str + "\"");

			if (indices.size() < 2 || indices[1].empty())
				throw std::runtime_error("Missing texture coordinate index (" + fileName + ")");
			if (indices.size() < 3 || indices[2].empty())
				throw std::runtime_error("Missing vertex normal index (" + fileName + ")");

			MeshData::Index index;
			index.v = Utility::fromStringExcept<unsigned int>(indices[0]);
			index.t = Utility::fromStringExcept<unsigned int>(indices[1]);
			index.n = Utility::fromStringExcept<unsigned int>(indices[2]);
			index.c = 0; // no vertex colors in OBJ files

			if (index.v == 0 || index.t == 0 || index.n == 0)
				throw std::runtime_error("Invalid index: 0 (" + fileName + ")");

			// OBJ indices are 1-indexed
			--index.v;
			--index.t;
			--index.n;

			data.indices.push_back(index);
		}
	}

	MeshData parse(const File::Buffer& buf)
	{
		// A std::streambuf that points to a memory buffer
		struct MemoryReadBuf : public std::streambuf
		{
			MemoryReadBuf(char* s, const std::size_t n)
			{ setg(s, s, s + n); }
		};

		// Construct a std::istream that points to "buf.data"
		MemoryReadBuf rb(const_cast<char*>(&buf.data[0]), buf.data.size());
		std::istream stream(&rb);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		MeshData result;

		while (!stream.eof()) {
			switch (stream.peek()) {
				case 'v':
					parseV(stream, result);
					break;

				case 'f':
					parseF(stream, result, buf.fileName);
					break;

				// case 'u':
				// 		parseU(stream, mesh);
				// 		break;

				case '#': // comments
				default:
					break;
			}

			if (!stream.eof()) {
				// Skip until the next line
				stream.ignore(INT_MAX, '\n');
			}
		}

		return result;
	}

} // namespace

MeshPtr LoaderOBJ::load(const std::string& fileName)
{
	const File::Buffer buf = File::read(fileName);

	MeshData data = parse(buf);

	if (data.indices.size() % 3)
		throw std::runtime_error("Invalid number of faces (" + buf.fileName + ")");

	return MeshPtr(new Mesh(data));
}
