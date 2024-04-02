#include "common_msg_queue.h"

#ifdef _WIN32
#include <functional>
#endif

MessageItem::MessageItem(int msg_id, int first_param, int second_param, void *data)
	: m_msg_id(msg_id), m_first_param(first_param), m_second_param(second_param), m_pdata(data)
{
}

MessageItem::~MessageItem()
{
}

int MessageItem::get_id() const
{
	return this->m_msg_id;
}

int MessageItem::get_first_param() const
{
	return this->m_first_param;
}

int MessageItem::get_second_param() const
{
	return this->m_second_param;
}

void *MessageItem::get_data() const
{
	return this->m_pdata;
}

/*****************************************************************/
/*****************************************************************/

static void* pthread_run_func(void* ptr)
{
	SimpleMessageQueue* queue = (SimpleMessageQueue*)ptr;
	queue->run();
	return 0;
}

SimpleMessageQueue::SimpleMessageQueue(int max_count)
	: m_initialized(false), m_max_queue_length(max_count), m_is_running(false)
{
#ifdef _WIN32
#else
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_condi, NULL);
#endif
}

SimpleMessageQueue::~SimpleMessageQueue()
{
#ifdef _WIN32
#else
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_condi);
#endif
}

void SimpleMessageQueue::start()
{
	m_is_running = true;

#ifdef _WIN32
	m_running_thread = std::thread(std::bind(&SimpleMessageQueue::run, this));
	m_initialized = true;
#else
	int ret = pthread_create(&m_running_thread, NULL, pthread_run_func, this);
	if(ret == 0)
	{
		m_initialized = true;
	}else
	{
		m_is_running = false;
		m_initialized = false;
	}
#endif
}

void SimpleMessageQueue::stop()
{
	if (m_is_running)
	{
		m_is_running = false;
		put(MessageItem(EXIT_MSG_ID, 0, 0, NULL));

		if (m_initialized)
		{
#ifdef _WIN32
			m_running_thread.join();
#else
			pthread_join(m_running_thread, NULL);
#endif
		}
	}
}

void SimpleMessageQueue::run()
{
	while (m_is_running)
	{
		MessageItem msg;
		//get the message item
		get(msg);
		if (msg.get_id() != EXIT_MSG_ID)
		{
			handle_msg(msg);
		}
#ifdef _WIN32
		std::this_thread::yield();
#else
		pthread_yield();
#endif
	}

	clear_msg_queue();
}

int SimpleMessageQueue::get(MessageItem &msg)
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(this->m_mutex);

	this->m_condi.wait(lock, [this] {
		if (!m_msg_queue.empty())
		{
			return true;
		}
		else
		{
			return false;
		}
	});

	//get
	msg = m_msg_queue.front();
	m_msg_queue.pop_front();

	return m_msg_queue.size();
#else
	pthread_mutex_lock(&m_mutex);

	if(m_msg_queue.empty())
	{
		pthread_cond_wait(&m_condi, &m_mutex);
	}

	//get
	msg = m_msg_queue.front();
	m_msg_queue.pop_front();

	int size = m_msg_queue.size();

	pthread_mutex_unlock(&m_mutex);
	return size;
#endif
}

bool SimpleMessageQueue::put(const MessageItem &msg)
{
	if (!m_is_running && msg.get_id() != EXIT_MSG_ID)
	{
		destroy_msg(msg);
		return false;
	}
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(this->m_mutex);

	//add item
	if ((int)m_msg_queue.size() < m_max_queue_length)
	{
		m_msg_queue.push_back(msg);

		if (m_msg_queue.size() == 1)
		{
			this->m_condi.notify_one();
		}

		return true;
	}
	else
	{
		//destroy the msg
		if (msg.get_id() != EXIT_MSG_ID)
		{
			destroy_msg(msg);
		}
		return false;
	}
#else
	pthread_mutex_lock(&m_mutex);

	//add item
	if ((int)m_msg_queue.size() < m_max_queue_length)
	{
		m_msg_queue.push_back(msg);

		if (m_msg_queue.size() == 1)
		{
			pthread_mutex_unlock(&m_mutex);
			pthread_cond_signal(&m_condi);
		}else
		{
			pthread_mutex_unlock(&m_mutex);
		}
		
		return true;
	}
	else
	{
		pthread_mutex_unlock(&m_mutex);

		//destroy the msg
		if (msg.get_id() != EXIT_MSG_ID)
		{
			destroy_msg(msg);
		}

		return false;
	}
#endif
}

void SimpleMessageQueue::clear_msg_queue()
{
	//clear the remaining data
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(this->m_mutex);
	while (m_msg_queue.size() > 0)
	{
		MessageItem item = m_msg_queue.front();
		m_msg_queue.pop_front();
		if (item.get_id() != EXIT_MSG_ID)
		{
			destroy_msg(item);
		}
	}
#else
	pthread_mutex_lock(&m_mutex);
	while (m_msg_queue.size() > 0)
	{
		MessageItem item = m_msg_queue.front();
		m_msg_queue.pop_front();
		if (item.get_id() != EXIT_MSG_ID)
		{
			destroy_msg(item);
		}
	}
	pthread_mutex_unlock(&m_mutex);
#endif
}