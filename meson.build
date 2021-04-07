# uncomment as necessary
project('HTTP-Client_1.1', 'cpp',
        license : ['Boost'],
        version : '0.1',
        default_options : ['cpp_std=c++17', 'warning_level=3', 'werror=true'])

thread_dep = dependency('threads')

# begin asio: https://think-async.com/Asio/
add_global_arguments('-I' + get_option('asio_include_dir'), language : 'cpp')
add_global_arguments('-DASIO_STANDALONE', language : 'cpp')
add_global_arguments('-DASIO_NO_DEPRECATED', language : 'cpp')
# end asio

# CLI11: https://github.com/CLIUtils/CLI11
add_global_arguments('-I' + get_option('cli11_include_dir'), language : 'cpp')

# spdlog: https://github.com/gabime/spdlog
add_global_arguments('-I' + get_option('spdlog_include_dir'), language : 'cpp')

# json: https://github.com/nlohmann/json
add_global_arguments('-I' + get_option('json_include_dir'), language : 'cpp')

# rang
add_global_arguments('-I' + get_option('rang_include_dir'), language : 'cpp')





# add additional include global arguments appropriately!

# begin protobuf: https://developers.google.com/protocol-buffers/
# protobuf: must be installed independently and has to be found...
protoc = find_program('protoc', required : true)
protobuf_dep = dependency('protobuf', required : true)

gen = generator(protoc, \
output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/src', '--cpp_out=@BUILD_DIR@', '@PLAINNAME@'])

# # change proto file accordingly
generated = gen.process('src/HTTPResponseObject.proto')
# # end protobuf

# subdir('SUBDIR1')
inc_dir = include_directories(['include', get_option('asio_include_dir'), 
                                          get_option('spdlog_include_dir'), 
                                          get_option('cli11_include_dir'), 
                                          get_option('json_include_dir'),
                                          get_option('rang_include_dir')
                                          ])
src = ['src/main.cpp', generated]
#protobuf: src = ['src/main.cpp', generated]

executable('http_client',
           sources : src,
           include_directories : inc_dir,
           dependencies : [thread_dep, protobuf_dep]
          )
