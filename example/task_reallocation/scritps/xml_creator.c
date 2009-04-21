#include <stdio.h>
#include <stdlib.h>

long unsigned min (long unsigned a, long unsigned b) {
  if (a < b) return a;
  return b;
}

double mind (double a, double b) {
  if (a < b) return a;
  return b;
}

int main(int argc, char * argv[]) {
  FILE* platform;
  FILE* deployment;
  FILE* simbatch;
  int batch_number = 3;
  int node_number = 40;
  //power of each node in each cluster
  long unsigned power[] = {800000, 1000000, 1200000};
  //BW for loopback in each cluster
  long unsigned loopback[] = {498000000, 498000000, 498000000};
  //internal BW in each cluster
  long unsigned bw[] = {100000000, 100000000, 100000000};
  //external BW between each cluster
  long unsigned externalBW[] = {100000000, 100000000, 100000000};
  double externalLatency[] = {0.0, 0.0, 0.0};
  //client BW
  long unsigned clientBW = 100000000;
  double clientLatency = 0.0;
  int i;
  int j;

  platform = fopen("platform.xml", "w");

  fprintf(platform, "<?xml version='1.0'?>\n"
	  "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n"
	  "<platform_description version=\"1\">\n");

  //Batchs
  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "\n  <!-- Cluster %d -->\n", i);
    fprintf(platform, "  <cpu name=\"Batch%d\" power=\"%lu\"/>\n", i, power[i]);
    for (j = 0; j < node_number; j++) {
      fprintf(platform, "  <cpu name=\"Node%d_%d\" power=\"%lu\"/>\n", 
	      i, j, power[i]);
    }
    fprintf(platform, "\n  <network_link name=\"loopback%d\""
	    " bandwidth=\"%lu\" latency=\"0.0\"/>\n", i, loopback[i]);
    for (j = 0; j < node_number; j++) {
      fprintf(platform, "  <network_link name=\"B%d_%d\""
	      " bandwidth=\"%lu\" latency=\"0.0\"/>\n", i, j, bw[i]);
    }

    fprintf(platform, "\n  <route src=\"Batch%d\" dst=\"Batch%d\">"
	    "<route_element name=\"loopback%d\"/></route>\n", i, i, i);
    for (j = 0; j < node_number; j++) {
      fprintf(platform, "  <route src=\"Batch%d\" dst=\"Node%d_%d\">"
	      "<route_element name=\"B%d_%d\"/></route>\n", i, i, j, i, j);  
    }
    for (j = 0; j < node_number; j++) {
      fprintf(platform, "  <route src=\"Node%d_%d\" dst=\"Batch%d\">"
	      "<route_element name=\"B%d_%d\"/></route>\n", i, j, i, i, j);  
    }

  }

  //metaSched
  fprintf(platform, "\n  <!-- MetaScheduler -->\n");
  fprintf(platform, "  <cpu name=\"MetaSched\" power=\"3865000000\"/>\n\n");
  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <network_link name=\"M%d\""
	    " bandwidth=\"1000000000\" latency=\"%lf\"/>\n", i, 
	    externalLatency[i]);
  }

  fprintf(platform, "\n");

  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <route src=\"MetaSched\" dst=\"Batch%d\">"
	    "<route_element name=\"M%d\"/></route>\n", i, i);  
  }

  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <route src=\"Batch%d\" dst=\"MetaSched\">"
	    "<route_element name=\"M%d\"/></route>\n", i, i);  
  }

  //Client
  fprintf(platform, "\n  <!-- Client -->\n");
  fprintf(platform, "  <cpu name=\"Client\" power=\"3865000000\"/>\n\n");

  fprintf(platform, "  <network_link name=\"CM\""
	  " bandwidth=\"%lu\" latency=\"%lf\"/>\n", clientBW, clientLatency);
  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <network_link name=\"C%d\""
	    " bandwidth=\"%lu\" latency=\"%lf\"/>\n", i, clientBW, 
	    clientLatency);
  }

  fprintf(platform, "\n  <route src=\"Client\" dst=\"MetaSched\">"
	  "<route_element name=\"CM\"/></route>\n");
  fprintf(platform, "  <route src=\"MetaSched\" dst=\"Client\">"
	  "<route_element name=\"CM\"/></route>\n");
  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <route src=\"Client\" dst=\"Batch%d\">"
	    "<route_element name=\"C%d\"/></route>\n", i, i);  
  }
  
  for (i = 0; i < batch_number; i++) {
    fprintf(platform, "  <route src=\"Batch%d\" dst=\"Client\">"
	    "<route_element name=\"C%d\"/></route>\n", i, i);  
  }

  //Between Clusters
  fprintf(platform, "\n  <!-- Between clusters -->\n");
  for (i = 0; i < batch_number; i++) {
    for (j = i + 1; j < batch_number; j++) {
      long unsigned m = min(externalBW[i], externalBW[j]);
      double d = mind(externalLatency[i], externalLatency[j]);
      fprintf(platform, " <network_link name=\"Ba%d_%d\""
	      " bandwidth=\"%lu\" latency=\"%lf\"/>\n", i, j, m, d);
    }
  }

  fprintf(platform, "\n");
  
  for (i = 0; i < batch_number; i++) {
    for (j = i + 1; j < batch_number; j++) {
      fprintf(platform, " <route src=\"Batch%d\" dst=\"Batch%d\">"
	      "<route_element name=\"Ba%d_%d\"/></route>\n", i, j, i, j);
    }
  }
  
  for (i = 0; i < batch_number; i++) {
    for (j = i + 1; j < batch_number; j++) {
      fprintf(platform, " <route src=\"Batch%d\" dst=\"Batch%d\">"
	      "<route_element name=\"Ba%d_%d\"/></route>\n", j, i, i, j);
    }
  }
  
  fprintf(platform, "\n</platform_description>\n");
  
  fclose(platform);
  
  deployment = fopen("deployment.xml", "w");
  
  fprintf(deployment, "<?xml version='1.0'?>\n"
	  "<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n"
	  "<platform_description version=\"1\">\n");

  fprintf(deployment, "\n  <process host=\"Client\" function=\"client\">\n"
	  "    <argument value=\"jobs.txt\"/>\n"
	  "    <argument value=\"MetaSched\"/>\n"
	  "  </process>\n");

  fprintf(deployment, "\n  <process host=\"MetaSched\""
	  " function=\"metaSched\">\n");
  fprintf(deployment, "    <argument value=\"Client\"/>\n");
  for (i = 0; i < batch_number; i++) {
    fprintf(deployment, "    <argument value=\"Batch%d\"/>\n", i);
  }
  fprintf(deployment, "  </process>\n");
  
  fprintf(deployment, "\n  <process host=\"MetaSched\" function=\"resched\">\n"
	  "    <argument value=\"100\"/>\n"
	  "    <argument value=\"100000\"/>\n"
	  "  </process>\n");

  for (i = 0; i < batch_number; i++) {
    fprintf(deployment, "\n  <!-- Cluster %d -->\n", i);
    fprintf(deployment, "  <process host=\"Batch%d\" function=\"sed\">\n"
	    "    <argument value=\"MetaSched\"/>\n"
	    "    <argument value=\"Batch%d\"/>\n"
	    "    <argument value=\"perfs.txt\"/>\n"
	    "  </process>\n", i, i);
    fprintf(deployment, "  <process host=\"Batch%d\""
	    " function=\"SB_batch\">\n", i);
    for (j = 0; j < node_number; j++) {
      fprintf(deployment, "    <argument value=\"Node%d_%d\"/>\n", i, j);
    }
    fprintf(deployment, "  </process>\n");
    
    for (j = 0; j < node_number; j++) {
      fprintf(deployment, "  <process host=\"Node%d_%d\""
	      " function=\"SB_node\"/>\n", i, j);
    }

  }
  fprintf(deployment, "\n</platform_description>\n");

  fclose(deployment);

  simbatch = fopen("simbatch.xml", "w");

  fprintf(simbatch, "<config>\n\n");
  fprintf(simbatch, "  <global>\n"
	  "    <file type=\"platform\">platform.xml</file>\n"
	  "    <file type=\"deployment\">deployment.xml</file>\n"
	  "  </global>\n");

  for (i = 0; i < batch_number; i++) {
    fprintf(simbatch, "\n  <batch host=\"Batch%d\">\n"
	    "    <plugin>libfcfs.so</plugin>\n"
	    "    <priority_queue>\n"
	    "      <number>3</number>\n"
	    "    </priority_queue>\n"
	    "  </batch>\n", i);
  }

  fprintf(simbatch, "</config>\n");

  fclose(simbatch);

  return EXIT_SUCCESS;
}
