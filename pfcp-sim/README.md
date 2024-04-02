# tools/pfcp-sim
  - Compilation:
      ```
        make all
      ```

  - SMF simulator :
    
    How to run:
    ```
      #./smf_sim <peer-ip> <seq-no-seed>
      ./smf_sim 31.31.31.1 1000
      
      1 for assoc setup req
      2 for assoc rel req
      3 for heartbeat 
      4 for session est req
      5 for session mod req
      6 for session del req
      7 for session est-mod-del req
      8 for exit

    ```
  - UPF simulator :

    How to run:
    ```
     #./upf_sim <peer-ip> <seq-no-seed>
     ./upf_sim 20.20.20.1 3000

     1 for assoc setup req
     2 for assoc rel req
     3 for heartbeat 
     8 for exit

    ```
