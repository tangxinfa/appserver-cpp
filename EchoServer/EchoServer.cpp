#include "../FastCgiServer.hpp"
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

class EchoServer : public FastCgiServer
{
public:
    EchoServer()
    {
    }
    
    void handle(const HttpRequest& request, HttpResponse& response)
    {
        response << "Content-type: text/plain; charset=utf-8\r\n\r\n"
                 << request("msg");
    }
};

int main(int argc, char *argv[])
{
    size_t thread_num = boost::thread::hardware_concurrency();
    
    try
    {
        boost::program_options::options_description options("command line options");
        options.add_options()
            ("help,h", "use -h or --help to list all arguments")
            ("thread_num,t", boost::program_options::value<std::size_t>(&thread_num)->default_value(thread_num), "number of threads process request concurrently");
        boost::program_options::variables_map variables_map;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), variables_map);
        boost::program_options::notify(variables_map);
        if(variables_map.count("help"))
        {
            std::cout << options << std::endl;
            return 0;
        }        
    }catch(std::exception& e)
    {
        std::cerr << "parse command line error: " << e.what() << std::endl;
        ::exit(1);
    }

    EchoServer server;
    boost::thread_group threads;
    for(size_t i = 0; i < thread_num; ++i)
    {
        threads.create_thread(boost::bind(&EchoServer::run, &server));
    }
    threads.join_all();
    return 0;
}
