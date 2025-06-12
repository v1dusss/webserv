//
// Created by Emil Ebert on 12.06.25.
//

#ifndef INTERNALAPI_H
#define INTERNALAPI_H
#include <memory>
#include <parser/http/HttpRequest.h>
#include <server/response/HttpResponse.h>


namespace InternalApi {
    void registerRoutes(ServerConfig &serverConfig);
}


#endif //INTERNALAPI_H
