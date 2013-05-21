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
        FCGX_Request raw_request;
        FCGX_InitRequest(&raw_request, 0, 0);
        
        while(FCGX_Accept_r(&raw_request) == 0)
        {
            HttpResponse response(raw_request);
            HttpRequest request(&response);
            try{
                handle(request, response);
            }catch(std::exception& e){
                response.err() << "Exception: " << e.what() << std::endl;
                throw;
            }
            FCGX_Finish_r(&raw_request);
        }
    }
};

#endif//FASTCGISERVER_HPP_
