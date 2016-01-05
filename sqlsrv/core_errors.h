#ifndef __CORE_ERRORS__
#define __CORE_ERRORS__

#include "core_sqlsrv.h"
#include "core_memory.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <sql.h>

namespace ErrorConsts
{
	const std::size_t MAX_ERROR_COUNT = 32;
	const std::size_t MAX_ERROR_MESSAGE_SIZE = SQL_MAX_MESSAGE_LENGTH+1;
	enum class ErrorType { ODBC_ERROR, ODBC_WARNING };
	enum class RequestType { ALL, ERRORS, WARNINGS };
}

using ErrorMessage = std::array<char, ErrorConsts::MAX_ERROR_MESSAGE_SIZE>;

struct Error
{
	long native_code;
	ErrorConsts::ErrorType type;
	ErrorMessage msg;
	ErrorMessage sql_state;

	Error() : native_code{ 0 }, type{ ErrorConsts::ErrorType::ODBC_ERROR }
	{
		std::for_each(&msg[0], &msg[ErrorConsts::MAX_ERROR_MESSAGE_SIZE - 1],
			[&](char ch)
			{
				ch = '\0';
			}
		);

		std::for_each(&sql_state[0], &sql_state[ErrorConsts::MAX_ERROR_MESSAGE_SIZE - 1],
			[&](char ch)
		{
			ch = '\0';
		}
		);
	}
};

inline void copy_to_error_message(const unsigned char* source, ErrorMessage& dest)
{
	auto len = strlen((const char *)source);
	SQLSRV_ASSERT(len <= ErrorConsts::MAX_ERROR_MESSAGE_SIZE, "Error message size not supported");
	std::copy(source, source + len - 1, dest.begin());
}

class ErrorManager
{
	public :

		ErrorManager() : m_error_count{ 0 }
		{
		}

		void reset() { m_error_count = 0; }

		std::size_t count() const
		{
			return m_error_count;
		}

		void add_error(long code, ErrorConsts::ErrorType type, const unsigned char* message, const unsigned char* sql_state)
		{
			m_errors[m_error_count].type = type;
			m_errors[m_error_count].native_code = code;

			auto len = strlen((const char *)message);
			std::copy(message, message + len - 1, m_errors[m_error_count].msg.begin());

			copy_to_error_message(message, m_errors[m_error_count].msg);
			copy_to_error_message(sql_state, m_errors[m_error_count].sql_state);

			m_error_count = ++m_error_count % ErrorConsts::MAX_ERROR_COUNT;// ErrorManager is basically a ring buffer
		}

		void get_errors_as_zval_array(zval * zv, ErrorConsts::RequestType requestType)
		{
			sqlsrv_new_array_and_init(zv);
			
			for (std::size_t i{ 0 }; i < m_error_count; i++ )
			{
				zval current_error;
				sqlsrv_new_array_and_init(&current_error);
				Error iter = m_errors[i];
				bool valid = true;

				switch (requestType)
				{
					case ErrorConsts::RequestType::ERRORS:
						if (iter.type == ErrorConsts::ErrorType::ODBC_WARNING)
						{
							valid = false;
						}
						break;

					case ErrorConsts::RequestType::WARNINGS:
						if (iter.type == ErrorConsts::ErrorType::ODBC_ERROR)
						{
							valid = false;
						}
						break;
					
					case ErrorConsts::RequestType::ALL :
					default:
						break;
				}

				if (valid)
				{
					add_assoc_long(&current_error, "code", iter.native_code);
					add_assoc_string(&current_error, "message", &iter.msg[0]);
					add_assoc_string(&current_error, "SQLSTATE", &iter.sql_state[0]);

					add_next_index_zval(zv, &current_error);
				}
			}
		}

		~ErrorManager() = default;

	private :
		std::size_t m_error_count;
		std::array<Error, ErrorConsts::MAX_ERROR_COUNT> m_errors;
		// NON COPYABLE , NON MOVABLE, avoiding boost non copyable
		ErrorManager(ErrorManager&) = delete;
		ErrorManager& operator=(ErrorManager& other) = delete;
		ErrorManager(ErrorManager&& other) = delete;
		ErrorManager& operator=(ErrorManager&& other) = delete;
};

#endif