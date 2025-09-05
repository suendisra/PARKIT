# PARKIT 0.2.7

### About
> This is the PARKIT application. It is an application designed to enable management of parking lots.<br />
> It provides a server side managerial service over UDP and TCP to allow clients to view parking lot statuses of individual spots.<br />
> The client side is allowed to request the server to override specific spot location statuses and reserve spots for<br />
> extended periods of time. The server side manages such requests and updates all clients with the most up-to-date information.

### Note
> This application assumes a closed (isolated) network and applies a periodic UDP broadcast for client applications to discover.<br />
> The broadcast is done in plain-text, and no special security measures are in place to prevent MITM attacks,<br />
> as the only information disseminated is information regarding parking lot statuses.<br />
> However, isolating the network on which this server and its clients operate is <b>highly</b> recommended.

### Output
> This is a Win32 executable (.exe)

### What's in it?
> <ul>
> <li>Windows GDI and GUI</li>
> <li>Networking via UDP and TCP</li>
> <li>Database communication via ODBC2 and Windows SQL API</li>
> </ul>

### Dependencies
> This application depends on external dynamically-linked libraries (DLLs) in order
> to properly execute.
>
> | Dependency | Version |
> | ---------- | ------- |
> | GPH        | 0.603   |
> | NET        | 0.603   |
> | UTIL       | 0.603   |
> | WIN        | 0.603   |
