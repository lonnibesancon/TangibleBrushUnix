#include "loader_ply.h"

#include "rendering/mesh.h"
#include "util/file.h"

#include <sstream>
#include <streambuf>
#include <climits>

namespace
{
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
		// stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		MeshData result;

		std::string line;
		bool headerEnded = false;

		while (std::getline(stream, line)) {
			const std::vector<std::string> tokens = Utility::split(line, ' ');

			if (tokens.empty())
				throw std::runtime_error("Empty line (" + buf.fileName + ")");

			const std::string keyword = tokens[0];

			if (!headerEnded) {
				if (keyword == "end_header") {
					headerEnded = true;

				} else if (keyword != "ply" && keyword != "format"
				           && keyword != "comment" && keyword != "element"
				           && keyword != "property")
				{
					throw std::runtime_error("Unknown keyword: \"" + keyword + "\" (" + buf.fileName + ")");
				}

			} else if (keyword == "3") {
				if (tokens.size() < 4)
					throw std::runtime_error("Not enough indices for one face (" + buf.fileName + ")");

				for (int i = 1; i < 4; ++i) {
					MeshData::Index index;
					index.v = index.t = index.n = index.c =
						Utility::fromStringExcept<unsigned int>(tokens[i]);
					result.indices.push_back(index);
				}

			} else { // keyword != "3"
				if (keyword == "4")
					throw std::runtime_error("Quad faces are not allowed (" + buf.fileName + ")");

				if (tokens.size() < 8)
					throw std::runtime_error("Missing data (" + buf.fileName + ")");
				else if (tokens.size() >= 8 && tokens.size() < 11)
					throw std::runtime_error("Missing color components (" + buf.fileName + ")");

				std::istringstream iss(line);
				iss.exceptions(std::ifstream::failbit | std::ifstream::badbit);

				float x, y, z;
				iss >> x >> y >> z;
				result.vertices.push_back(Vector3(x, y, z));

				float nx, ny, nz;
				iss >> nx >> ny >> nz;
				result.normals.push_back(Vector3(nx, ny, nz));

				float u, v;
				iss >> u >> v;
				result.texCoords.push_back(Vector2(u, 1.0f - v));

				if (tokens.size() == 11) {
					unsigned int r, g, b;
					iss >> r >> g >> b;
					result.colors.push_back(Vector3(r, g, b) / 255.0f);
				}
			}
		}

		return result;
	}

} // namespace

MeshPtr LoaderPLY::load(const std::string& fileName)
{
	const File::Buffer buf = File::read(fileName);

	MeshData data = parse(buf);

	if (data.indices.size() % 3)
		throw std::runtime_error("Invalid number of faces (" + buf.fileName + ")");

	return MeshPtr(new Mesh(data));
}
