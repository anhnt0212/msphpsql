# PHP7 Port of Microsoft Drivers for PHP for SQL Server

**Welcome to PHP7 port of Microsoft Drivers for PHP for SQL Server!**

We are excited about PHP7 and use Microsoft`s SQLServer driver for PHP. Since it is not currently available for PHP7, 
we have created a port for PHP7. For the original project please visit https://github.com/azure/msphpsql .
Please note that this is in its initial stage and please see the section below regarding what is currently supported.

Thomson Reuters FATCA Technology Team

## List of supported functionality and features

1. Currently we are not porting the PDO_SQLSRV extension and targeting only the SQLSRV extension.

2. This port also includes Robert Johnson`s patch which we have been using and can be found at :
	
		http://robsphp.blogspot.com/2012/06/unofficial-microsoft-sql-server-driver.html

3. As for our own project, we have currently ported a limited test of SQLSRV functions , which can be seen as below : 
				
			sqlsrv_connect
			sqlsrv_close
			sqlsrv_errors
			sqlsrv_prepare
			sqlsrv_free_stmt
			sqlsrv_execute
			sqlsrv_num_field
			sqlsrv_next_result
			sqlsrv_fetch_array
			sqlsrv_configure
			sqlsrv_query
			sqlsrv_num_rows
			sqlsrv_field_metadata

4. As we found issues with the brand new PHP7 Zend memory manager, our current port doesn`t support 
the Zend memory manager. In order to use,  you will have to set USE_ZEND_ALLOC environment variable to zero,
otherwise the extension will fail during initialisation. As we are working on this , we are planning
to remove that limitation as soon as possible.

## Prerequisites 

You will need to install Visual C++ 2015 runtime :
https://www.microsoft.com/en-us/download/details.aspx?id=48145

## Build

The instructions are the same as those for the original driver , which can be found here :
https://github.com/Azure/msphpsql

## Source code information

As mentioned above , this project also includes another unofficial patch. Other than that you can search for "//PHP7 Port" in the project to see the PHP7 considerations.
We also have added a new file ( zend_utility.h ) to the project which contains a few macros and functions for porting considerations and also for exploring new PHP7 engine.

## License

The Microsoft Drivers for PHP for SQL Server are licensed under the MIT license.  See the LICENSE file for more details.
