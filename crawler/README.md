# Certificate Crawler
## Compilation
### Preparation 
To compile the crawler, you need to first compile and install the htmlstreamparser, which can be found in the path: `/depedent_library`.

The compilation is quite simple if you already installed all necessary tools such as `make`, `gcc`, etc.

You can find two Makefiles in this directory: `Makefile.local.example`, `Makefile.planetlab.example`.

The `Makefile.local.example` is placed for the local debugging and testing, and the `Makefile.planetlab.example` is for deploying to the remote environment. 

### Massively Deployment of Crawler
**If you just use the crawler locally, please just jum to next paragraph.** 
In my project, I deploy the crawler to the nodes of the distributed network Planetlab Node.
I don't wish to install dependencies for each planetlab node, so I placed .so file in the same directory of the crawler.
By doing so, I could just use a script to copy my crawler and depdencies to remote and directly deploy it without installation. 
The trick is that **I predefined, in Makefile, the path that crawler to find dependencies.** 
If you open the `Makefile.planetlab.example`, you may find the corresponding code from the line 10. 
So, if you have the same problem as me which you need to distribute the crawler to a massive number of node, and also you don't wish to install dependencies for them, here is the answer, just modify the library path in Makefile that is defined by `-rpath`.

**It is noteworhy, you need to make sure that the server you wish to deploy is the same as what you complie the code. Otherwise, the crawler won't run!**

Eventually, you need to rename the `Makefile.planetlab.example` to `Makefile`, and properly set the rpath.

### Local Deployment of Crawler
Local deployment is quite simple, you just rename the `Makefile.local.example` to `Makefile`

### Final Step

Now, you know what makefile you need to use. Then, you simply shoot the command `make`.
Then, the code would generate the executable file named as `CertCrawler` if everything is correctly configured.


## Execute
The code would ouput the result via HTTP protocal. The general idea is that the code deploys the CURL commandline and use **POST method** to submit the raw data to a web interface. You may find the web interface in this path `/database_interface`.

Here is an example to execute the crawler

```
./CertCrawler LABEL DOMAIN_LIST CRAWLING_DEPTH NUMBER_OF_MAX_RECORD THREAD_NUMBER DATA_PIPELINE_URL
```

Here is the explanations of each **parameter**:

**LABEL**

here, we wish to let the database keep track on what task that we are working on. There could be multiple deployments of this crawler, each of which is working on a different of tasks. We use a label to tell what task each crawler is working. The crawler would simply pass the label to the database.

**DOMAIN\_LIST**

This is a quite strait-forward name. Here, we give a name of file where crawler would read the seed domains from. We provide a baby sample in this path: /data_sample

**NUMBER\_OF\_MAX\_RECORD**

This variable defines the max number of seeds that crawler would read from the seeds file.

**THREAD\_NUMBER**

This variaable defines the max number of threads that will be used to concurrently crawling.

**DATA\_PIPELINE\_URL**

This variable defines the url of date pipeline where crawler will post the data to.


