# tools/pfcp-sim
  - SMF simulator : pfcp_udp_client.c
    
    Compilation:
      ```
        gcc pfcp_udp_client.c -o smf_sim
      ```
    How to run:
    ```
      #./smf_sim <seq-no-seed>
      ./smf_sim 1000
      
      1 for assoc setup req
      2 for assoc rel req
      3 for heartbeat 
      4 for session est req
      5 for session mod req
      6 for session del req
      7 for session est-mod-del req
      8 for exit

    ```
  - UPF simulator : pfcp_udp_server.c

    Compilation:
      ```
        gcc pfcp_udp_server.c -o upf_sim
      ```
    How to run:
    ```
      ./upf_sim
    ```
