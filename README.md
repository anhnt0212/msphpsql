# PHP7 Port of Microsoft Drivers for PHP for SQL Server

**Welcome to PHP7 port of Microsoft Drivers for PHP for SQL Server!**

As we are excited about PHP7 and use Microsoft`s SQLServer driver for PHP and since it is not currently available for PHP7, 
we have created a port for PHP7. For the original project please visit https://github.com/azure/msphpsql .
Please note that this is initial stage and please see the section below regarding what is currently supported.


Microsoft has published the source code to the driver on this site. With each successive update, we will revisit this plan. We've seen too many projects over-reach in their plans to be responsive to the community. We would prefer to start with a more conservative approach and make sure we're successfully delivering on that before expanding. We understand that many developers will download the source code to create their own build(s) of the driver. However, Microsoft supports only the Microsoft signed versions of the driver.

Thomson Reuters FATCA Technology Team

## List of supported functionality and features

1. Currently we are not porting PDO_SQLSRV extension and targeting only SQLSRV extension.

2. This port also includes Robert Johnson`s patch we have been using which can be found at :
	
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

4. As we found issues with brand new PHP7 Zend memory manager, our current port doesn`t support 
Zend memory manager. In order to use you have to set USE_ZEND_ALLOC environment variable to zero.
Otherwise the extension will fail during initialisation. As we are working on this , we are planning
to remove that limitation as soon as possible.

## Prerequisites 

You will need to install Visual C++ 2015 runtime :
https://www.microsoft.com/en-us/download/details.aspx?id=48145

## Build

The instructions are not different from the original driver , which can be seen here :
https://github.com/Azure/msphpsql

## Source code information

As mentioned above , this project also includes another unofficial patch. Except from that you can search for "//PHP7 Port" in the project to see the PHP7 considerations.
We also have added a new file ( zend_utility.h ) to the project which contains a few macros and functions for porting considerations and also for exploring new PHP7 engine.

## License

The Microsoft Drivers for PHP for SQL Server are licensed under the MIT license.  See the LICENSE file for more details.
