#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <tuple>


#ifdef TEST_OUT
#define INFO(req, what) do {std::cout << req << "\t " << what << "\t\t " << std::this_thread::get_id() << std::endl;} while (0)
#else
#define INFO(req, what)
#endif


template<typename TReq, typename TRep>
class LruCache {

public:
    LruCache(size_t size) : m_max_size(size) {}

    virtual ~LruCache() = default;
    LruCache(const LruCache&) = delete;
    LruCache(LruCache&&) = delete;
    LruCache& operator=(LruCache&) = delete;
    LruCache& operator=(LruCache&&) = delete;

    TRep make_request(const TReq& req) {
        {
            // search in cache
            std::lock_guard guard(m_mtx);

            if (auto it = m_cache.find(req); it != m_cache.end()) {
                INFO(it->first, "found");
                update_request_queue(it);

                return std::get<0>(it->second);
            }
        }

        return add_new_item_in_cache(req);
    }

protected:
    virtual TRep prepare_reply(const TReq& req) const;

private:
    const size_t m_max_size = 0;

    struct DeclT {
        using CacheType = std::map<TReq, std::tuple<TRep, DeclT>>;
        using QueueType = std::list<typename CacheType::iterator>;

        typename QueueType::iterator it;
    };

    using CacheIterator = typename DeclT::CacheType::iterator;

    // key: request
    // value: tuple(reply, iterator_to_queue)
    typename DeclT::CacheType m_cache;

    // queue of request, each item is reference to request in map
    typename DeclT::QueueType m_req_queue;

    std::mutex m_mtx;

    void update_request_queue(CacheIterator & cache_it) {
        // guard outside

        auto& queue_it = std::get<1>(cache_it->second).it;

        if (queue_it != m_req_queue.end()) {
            m_req_queue.erase(queue_it);
        }
        m_req_queue.push_front(cache_it);
        queue_it = m_req_queue.begin();
    }

    TRep add_new_item_in_cache(const TReq& req) {
        TRep reply = prepare_reply(req);

        {
            std::lock_guard guard(m_mtx);

            auto [it, added] = m_cache.emplace(req, std::make_tuple(TRep(), DeclT{m_req_queue.end()}));
            if (added) {
                // may be inserted in another thread
                std::get<0>(it->second) = std::move(reply);
            }
            update_request_queue(it);

            INFO(req, "added");

            remove_item_from_cache_if_need();

            return std::get<0>(it->second);
        }
    }

    void remove_item_from_cache_if_need() {
        // guard outside

        assert(m_req_queue.size() == m_cache.size() && "Implementation error!");

        if (m_cache.size() > m_max_size) {
            auto it = m_req_queue.back();
            m_req_queue.pop_back();

            assert(it != m_cache.end() && "Implementation error!");
            m_cache.erase(it);
            INFO(it->first, "remov");
        }
    }
};