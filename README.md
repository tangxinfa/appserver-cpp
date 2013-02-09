appserver-cpp
=============

a simple but powerfull application server write by c++ with exist libraries


EchoServer
----------
* config and restart nginx.

    server {
        listen       80;
        server_name  localhost;
        ...
        location ~ /EchoServer$ {
            root           "/usr/local/EchoServer";
            fastcgi_pass   127.0.0.1:8000;
            fastcgi_param  SCRIPT_FILENAME  "/usr/local/EchoServer/$fastcgi_script_name";
            include        fastcgi_params;
        } 
        ...
    }

* start fastcgi application.

  /usr/local/EchoServer/start.sh

* test with browser.

  http://localhost/EchoServer?msg=hello 

* test with test application.

  /usr/local/EchoServer/test/test 127.0.0.1 80