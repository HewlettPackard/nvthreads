#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>

boost::atomic_int threads_finished(0);
void dowork(int i) {
        std::cout << "hello Iam thread with thread ID :" << boost::this_thread::get_id()<<"\n";
threads_finished++;
}

int main()
{
        boost::thread_group threadpool;
        int numTasks = 10;
        int numThreads = 3;
	boost::shared_ptr< boost::asio::io_service > ioservice(
             new boost::asio::io_service);
        //work object
        	boost::shared_ptr< boost::asio::io_service::work > work(
                new boost::asio::io_service::work( *ioservice ));
     
                for(int i = 0; i < numThreads; i++) {
                     threadpool.create_thread(
                     boost::bind(&boost::asio::io_service::run, ioservice));
               }
        
	for(int j = 0; j < numTasks; j++) {
                std::cout<<"Task number"<<j<<"\n";
                threads_finished = 0;

                for(int i=0; i < numThreads; i++) {
                        ioservice->post(boost::bind(dowork, i));
                }
    while(threads_finished < numThreads){
                }
        }
        work.reset();
        ioservice->stop();
	threadpool.join_all();
}

