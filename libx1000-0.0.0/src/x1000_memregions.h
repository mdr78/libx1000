
struct dseg {
        uint64_t data;
	uint64_t size;
	uint32_t type;
        struct dseg *next;
};

int get_mlayout(struct dseg **playout);
