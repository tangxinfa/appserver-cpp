#ifndef FASTCGISERVER_HPP_
#define FASTCGISERVER_HPP_

#include <cgicc/Cgicc.h>
#include <FCgiIO.h>

typedef cgicc::Cgicc HttpRequest;
typedef cgicc::FCgiIO HttpResponse;

class FastCgiServer
{
public:
    FastCgiServer()
    {
        FCGX_Init();        
    }

    virtual ~FastCgiServer()
    {
    }

    virtual void handle(const HttpRequest& request, HttpResponse& response) = 0;

    void run()
    {
        while(true)
        {
            FCGX_Request raw_request;
            if(FCGX_InitRequest(&raw_request, 0, 0) == 0 &&
               FCGX_Accept_r(&raw_request) == 0)
            {
                HttpResponse response(raw_request);
                HttpRequest request(&response);
                handle(request, response);
                FCGX_Finish_r(&raw_request);
            }
        }
    }
};

#endif//FASTCGISERVER_HPP_
