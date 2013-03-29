appserver-cpp
=============

a simple but powerfull application server framework write by c++ with exist libraries


EchoServer
----------
implementation echo server with appserver-cpp framework.

* config and restart nginx.
        worker_processes 4;

        upstream EchoServer_backend {
            server 127.0.0.1:8000;
            keepalive 3; # Make sure "keepalive*worker_processes < appserver threads", otherwise high concurrency(means >keepalive*worker_processes) will cause DOS.
        }

        server {
            listen       80;
            server_name  localhost;
            ...
            location ~ /EchoServer$ {
                root           "/usr/local/EchoServer";
                fastcgi_keep_conn on;
                fastcgi_send_timeout 5s;
                fastcgi_read_timeout 5s;
                fastcgi_connect_timeout 5s;
                fastcgi_pass   EchoServer_backend;
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
