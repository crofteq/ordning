set(PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/aboutdialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/aboutdialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/agreementtab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/agreementtab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/agreementtablemodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/agreementtablemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/companytab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/companytab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/customertab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/customertab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/customertablemodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/customertablemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicedrafttab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicedrafttab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicedrafttablemodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicedrafttablemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicefactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicefactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoiceprinter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoiceprinter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicetab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicetab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicetablemodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invoicetablemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/customer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/customer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/companyinfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/companyinfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/database.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/business.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/business.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/agreement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/agreement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/customers.h data/customers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/agreements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/agreements.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoicedraft.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoicedraft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoicedrafts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoicedrafts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoice.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoices.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/invoices.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/idatastore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/sqlitedatastore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/data/sqlitedatastore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/printerutilities.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/printerutilities.cpp
)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/clients
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_CURRENT_SOURCE_DIR}/util
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PdfWidgets
    Qt6::Svg
    Qt6::PrintSupport
    Qt6::SvgWidgets
    Qt6::Network
    Qt6::Sql
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

qt_finalize_executable(${PROJECT_NAME})
