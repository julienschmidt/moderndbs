#ifndef TID_H_
#define TID_H_

struct TID {
    // DO NOT change the order (using TID{...})
    uint32_t pageID;
    uint32_t slotID;

    bool operator==(const TID& tid) const {
        return pageID == tid.pageID && slotID == tid.slotID;
    }

    bool operator<(const TID& tid) const {
        return pageID < tid.pageID || (pageID == tid.pageID && slotID < tid.slotID);
    }
};

// implement hash func for std::hash
namespace std {
    template<>
    struct hash<TID> {
        typedef TID argument_type;
        typedef std::size_t value_type;

        value_type operator()(argument_type const& tid) const {
            value_type const h1 ( std::hash<uint32_t>()(tid.pageID) );
            value_type const h2 ( std::hash<uint32_t>()(tid.slotID) );
            return h1 ^ (h2 << 1);
        }
    };
}

#endif  // TID_H_
