# kcut tcp test program

We use a simple tcp non-blocking echo server and client written by Claude as our target application to
test and develop simple tcp short cuts

By default the server and client communicate using port 8080.  Don't forget to confirm that you have
the port open on your host/vm where the server is running.

On fedora to open the port you would do something like:

```
$ sudo firewall-cmd --permanent --add-port=8080/tcp
$ sudo firewall-cmd --reload
```

