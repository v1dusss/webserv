#include "RequestHandler.h"
#include <fstream>
#include <filesystem>
#include <sstream>
// TODO: refactor to smart buffer
HttpResponse RequestHandler::handlePut() const {
    // raw filename from URI
    /*
    std::string raw = request.uri.substr(request.uri.find_last_of('/') + 1);
    std::string name = urlDecode(raw);

    namespace fs = std::filesystem;
    fs::path route(routePath);
    fs::path dir = route.parent_path();
    fs::path out = dir / name;

    std::ofstream ofs(out, std::ios::binary);
    if (!ofs.is_open()) {
        return HttpResponse(500);  // can’t open file
    }

    ofs.write(request.body.data(), request.body.size());
    if (ofs.fail()) {
        return HttpResponse(500);  // write error
    }

*/
    return HttpResponse(201);  // Created
}