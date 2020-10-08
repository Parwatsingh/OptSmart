# OptSmart: A Space Efficient Optimistic Concurrent Execution of Smart Contracts
Parwat Singh Anjana, Sweta Kumari, Sathya Peri, Sachin Rathor, Archit Somani

## Appeared 
In the 27th Euromicro International Conference on Parallel, Distributed and Network-Based Processing (PDP), 2019 [link to Conference website!](https://www.pdp2019.eu/index.html)
### An Efficient Framework for Optimistic Concurrent Execution of Smart Contracts, Parwat Singh Anjana, Sweta Kumari, Sathya Peri, Sachin Rathor, Archit Somani, [DOI](https://ieeexplore.ieee.org/document/8671637)

## Description
This repo consists of extended approach of the paper publised in PDP. There are 3 approaches in each contract for extended STM based work other then state-of-the-art (static alaysis based bin approach, speculative bin appraoch) and serial approach. The STM approaches are as:
(1) Default Approach
(2) Extended Approach-1 
(3) Extended Approach-2
### (1) Default Approach: 
This is the same approach as used in PDP.
### (2) Extended Approach-1: 
In this approach at the miner side, all the AUs are executed speculatively in parallel, and the concurrent bin is computed simultaneously on the fly. So the basic idea is that all the AUs are added in the concurrent bin in the beginning. If the conflict list of an AU is nonempty during the execution, then AU, along with all AUs in the conflict list, is removed from the concurrent bin. All the conflicting AUs will be added in the block graph (BG), while all the valid AUs that do not have any conflict will remain in the concurrent bin. Miner sends the concurrent bin AUs and optimized BG to the validator, including valid AUs (SCTs) added in the block.
At the validator side, there are two phases the phase one is the concurrent bin phase, and the second phase is BG-phase. In the first phase, the parallel thread executes all the AUs in the concurrent bin, while in the second phase, threads execute the AUs with BG's help.
### (3) Extended Approach-2: 
This approach is the same as Extended Approach-1, but at the miner side, the difference is that the concurrent bin is computed at the end of the execution AUs using BG. All the AUs (vertex node) in the BG having in-degree and out-degree as 0 will be added in the concurrent bin, and the corresponding vertex node of AU is removed from the BG.

## Copyright 2019 Parallel and Distributed Computing Research Lab (PDCRL), IIT Hyderabad, India
  Licensed under the Apache License, Version 2.0 (the "License"); you may not use files in this project except in
  compliance with the License. You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific 
  language governing permissions and limitations under the License.
