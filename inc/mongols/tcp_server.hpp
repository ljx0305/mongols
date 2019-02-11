#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP


#include <netinet/in.h>   
#include <sys/signal.h>
#include <string>
#include <utility>
#include <unordered_map>
#include <atomic>
#include <list>
#include <thread>
#include <queue>
#include <ctime>


#include "epoll.hpp"
#include "thread_pool.hpp"
#include "lib/LRUCache11.hpp"


#include "openssl.hpp"


#define CLOSE_CONNECTION true
#define KEEPALIVE_CONNECTION false

namespace mongols {

    class tcp_server {
    public:

        class client_t {
        public:

            client_t();

            client_t(const std::string& ip, int port, size_t uid, size_t gid);
            virtual~client_t() = default;
            std::string ip;
            int port;
            time_t t;
            size_t sid, uid, u_size, count;
            std::list<size_t> gid;
        };
        typedef std::function<bool(const client_t&) > filter_handler_function;
        typedef std::function<std::string(
                const std::pair<char*, size_t>&
                , bool&
                , bool&
                , client_t&
                , filter_handler_function&) > handler_function;


    public:
        tcp_server() = delete;
        tcp_server(const std::string& host, int port, int timeout = 5000
                , size_t buffer_size = 8192
                , int max_event_size = 64);
        virtual~tcp_server();

    public:
        void run(const handler_function&);

        size_t get_buffer_size()const;


        bool set_openssl(const std::string&, const std::string&
                , openssl::version_t v = openssl::version
                , const std::string& ciphers = openssl::ciphers
                , long flags = openssl::flags);

        void set_enable_blacklist(bool);


        static int backlog;
        static size_t backlist_size;
        static size_t max_connetion_limit;
        static size_t backlist_timeout;
    private:
        std::string host;
        int port, listenfd, max_event_size;
        struct sockaddr_in serveraddr;
        static std::atomic_bool done;
        static void signal_normal_cb(int sig, siginfo_t *, void *);
        void setnonblocking(int fd);
        void main_loop(struct epoll_event *, const handler_function&, mongols::epoll&);
    protected:

        class meta_data_t {
        public:
            meta_data_t();
            meta_data_t(const std::string& ip, int port, size_t uid, size_t gid);
            virtual~meta_data_t() = default;

        public:
            client_t client;
            std::shared_ptr<openssl::ssl> ssl;
        };

        class black_ip_t {
        public:
            black_ip_t();
            virtual~black_ip_t() = default;
            time_t t;
            size_t count;
            bool disallow;
        };
        size_t buffer_size, thread_size, sid;
        int timeout;
        std::queue<size_t, std::list<size_t>> sid_queue;
        std::unordered_map<int, meta_data_t > clients;
        mongols::thread_pool<std::function<bool() >> *work_pool;

        lru11::Cache<std::string, std::shared_ptr<black_ip_t>> blacklist;

        std::shared_ptr<mongols::openssl> openssl_manager;
        std::string openssl_crt_file, openssl_key_file;
        bool openssl_is_ok, enable_blacklist;

        virtual bool add_client(int, const std::string&, int);
        virtual void del_client(int);
        virtual bool send_to_all_client(int, const std::string&, const filter_handler_function&);
        virtual bool work(int, const handler_function&);
        virtual bool ssl_work(int, const handler_function&);
        virtual bool check_blacklist(const std::string&);
    };
}


#endif /* TCP_SERVER_HPP */

