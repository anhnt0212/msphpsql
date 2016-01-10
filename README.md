# PHP7 Port of Microsoft Drivers for PHP for SQL Server

**Welcome to PHP7 port of Microsoft Drivers for PHP for SQL Server!**

We are excited about PHP7 and use Microsoft`s SQLServer driver for PHP. Since it is not currently available for PHP7, 
we have created a port for PHP7. For the original project please visit https://github.com/azure/msphpsql .
Please note that this is in its initial stage and please see the section below regarding what is currently supported.

Thomson Reuters FATCA Technology Team

## List of supported functionality and features

1. We are not porting the PDO_SQLSRV extension and targeting only the SQLSRV extension.

2. We are only supporting non-ZTS mode of PHP7.

3. We are supporting only x86.

4. As for our own project, we have currently ported a limited set of SQLSRV functions , which can be seen as below : 
				
    1. sqlsrv_connect
    2. sqlsrv_close
    3. sqlsrv_errors
    4. sqlsrv_prepare
    5. sqlsrv_free_stmt
    6. sqlsrv_execute
    7. sqlsrv_num_field
    8. sqlsrv_next_result
    9. sqlsrv_fetch_array ( excluding streams )
    10. sqlsrv_configure
    11. sqlsrv_query
    12. sqlsrv_num_rows
    13. sqlsrv_field_metadata
    14. sqlsrv_client_info
    
## Remarks

Official sqlsrv_num_rows documentation on http://php.net/manual/en/function.sqlsrv-num-rows.php might be misleading.
If you look at the SQL Native Client documentation on https://msdn.microsoft.com/en-us/library/ms711835(v=vs.85).aspx 
, sqlsrv_num_rows in fact returns the number of rows affected.

## Prerequisites 

You will need to install Visual C++ 2015 runtime :
https://www.microsoft.com/en-us/download/details.aspx?id=48145

## Build & Binaries

The instructions are the same as those for the original driver , which can be found here :
https://github.com/Azure/msphpsql
The only different thing is you will need VS20015. You can also find the build DLL and the pdb file
in "binaries_php7_non_zts_x86" directory.

## Contact
For your questions, please contact akin.ocal@thomsonreuters.com

## Source code information


1. We centralised all memory allocations and frees in new core_memory.h file. We also added CRT debugging features to the same header file such as programatic
   memory breakpoints, CRT memory function hooks and CRT debug heap reporting. You can turn these on by adding SQLSRV_MEM_DEBUG=1 and SQLSRV_HOOK_CRT_MALLOC=1 to your preprocessor.
   Additionaly inside core_memory.h , there is a commented out Zend memory manager heap walker code. This has been useful for us since PHP7 doesn`t provide a heap integrity
   checker unlike PHP5.X.

2. We are using static memory with std::array for error and warning reporting : core_errors.h 

3. We added flags inside php_sqlsrv.h to switch between different resource managers : EG(regular_list) and a custom global one which is not being touched by PHP7 garbage collector. As we are still working for the custom manual resource manager, it is not advised to turn the custom one on for production environments.

4. We also have added a new file ( zend_utility.h ) to the project which contains a few macros and functions for porting considerations and also for exploring new PHP7 engine.

5. This port also includes Robert Johnson`s patch which we have been using and can be found at :
	
		http://robsphp.blogspot.com/2012/06/unofficial-microsoft-sql-server-driver.html

## License

The Microsoft Drivers for PHP for SQL Server are licensed under the MIT license.  See the LICENSE file for more details.
