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

    const TRep& make_request(const TReq& req) {
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

    using CacheType = std::map<TReq, std::tuple<TRep, int>>;
    using CacheIterator = typename CacheType::iterator;

    // key: request
    // value: tuple(reply, counter_of_requests)
    CacheType m_cache;

    // queue of request, each item is reference to request in map
    std::list<CacheIterator> m_req_queue;

    std::mutex m_mtx;

    void update_request_queue(CacheIterator& it) {
        // guard outside

        m_req_queue.push_front(it);
        std::get<1>(it->second)++;
    }

    const TRep& add_new_item_in_cache(const TReq& req) {
        TRep reply = prepare_reply(req);

        {
            std::lock_guard guard(m_mtx);

            auto [it, added] = m_cache.emplace(req, std::make_tuple(TRep(), 0));
            if (added) {
                // may be inserted in another thread
                std::get<0>(it->second) = std::move(reply);
            }
            update_request_queue(it);
            INFO(req, "added");

            if (m_cache.size() > m_max_size) {
                remove_item_from_cache();
            }

            return std::get<0>(it->second);
        }
    }

    void remove_item_from_cache() {
        // guard outside

        while (!m_req_queue.empty()) {
            auto it = m_req_queue.back();
            m_req_queue.pop_back();

            if (--std::get<1>(it->second) <= 0) {
                // found item to remove
                m_cache.erase(it);
                INFO(it->first, "remov");
                return;
            }
        }
        assert(false && "Implementation error!");
    }
};