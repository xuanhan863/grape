#ifndef __QUEUE_HPP
#define __QUEUE_HPP

#include "grape/elliptics_client_state.hpp"

#include <cocaine/framework/logging.hpp>

#include <elliptics/cppdef.h>
#include <map>

namespace ioremap { namespace grape {

struct chunk_entry {
	int		size;
	int		state;
};

struct chunk_disk {
	int			acked, used, max;
	struct chunk_entry	states[];
};

class chunk_ctl {
	public:
		ELLIPTICS_DISABLE_COPY(chunk_ctl);

		chunk_ctl(int max);

		bool push(int size); // Returns true when given chunk is full
		int ack(int pos, int state); // Marks entry at @pos position with @state state. Increases @ack and returns it

		std::string &data(void);
		void assign(char *data, size_t size);

		int used(void) const;
		int acked(void) const;

		struct chunk_entry operator[] (int pos);

	private:
		std::string m_chunk;
		struct chunk_disk *m_ptr;
};

struct chunk_stat {
	uint64_t		write_data_sync;
	uint64_t		write_data_async;
	uint64_t		write_ctl_sync;
	uint64_t		write_ctl_async;
	uint64_t		read;
	uint64_t		remove;
	uint64_t		push;
	uint64_t		pop;
	uint64_t		ack;
};

class chunk {
	public:
		ELLIPTICS_DISABLE_COPY(chunk);

		chunk(elliptics::session &session, const std::string &queue_id, int chunk_id, int max);
		~chunk();

		bool push(const elliptics::data_pointer &d); // returns true if chunk is full
		elliptics::data_pointer pop(void);

		void remove(void);

		struct chunk_stat stat(void);
		void add(struct chunk_stat *st);

	private:
		int m_chunk_id;
		elliptics::key m_data_key;
		elliptics::key m_ctl_key;
		elliptics::session m_session_data;
		elliptics::session m_session_ctl;

		struct chunk_stat m_stat;

		size_t m_pop_position;
		size_t m_pop_index;

		// whole chunk data is cached here
		// cache is being filled when ::pop is invoked and @m_pop_position is >= than cache size
		elliptics::data_pointer m_chunk_data;

		chunk_ctl m_chunk;

		void write_chunk(void);
};

typedef std::shared_ptr<chunk> shared_chunk;

struct queue_stat {
	uint64_t	push_count;
	uint64_t	pop_count;
	uint64_t	fail_count;
	uint64_t	ack_count;
	int		chunk_id_push;
	int		chunk_id_pop;

	uint64_t	update_indexes;

	struct chunk_stat	chunks_popped;
	struct chunk_stat	chunks_pushed;
};

class queue {
	public:
		ELLIPTICS_DISABLE_COPY(queue);

		queue(const std::string &config, const std::string &queue_id);

		void push(const elliptics::data_pointer &d);
		elliptics::data_pointer pop(void);

		void reply(const ioremap::elliptics::exec_context &context,
				const ioremap::elliptics::data_pointer &d,
				ioremap::elliptics::exec_context::final_state state);
		void final(const ioremap::elliptics::exec_context &context, const ioremap::elliptics::data_pointer &d);

		struct queue_stat stat(void);
		const std::string queue_id(void) const;

	private:
		int m_chunk_max;

		std::map<int, shared_chunk> m_chunks;

		std::string m_queue_id;
		std::string m_queue_stat_id;

		elliptics_client_state m_client;

		struct queue_stat m_stat;

		void update_indexes(void);
};

}} /* namespace ioremap::grape */

#endif /* __QUEUE_HPP */
