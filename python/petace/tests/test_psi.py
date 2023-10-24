# Copyright 2023 TikTok Pte. Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest
from multiprocessing import Process
import numpy as np

from petace.setops.psi import PSI
from petace.setops.pjc import PJC
from petace.setops.pysetops import NetParams, NetScheme
from petace.setops.pysetops import PSIScheme, PJCScheme


class TestPsi(unittest.TestCase):

    def run_ecdh_psi_process(self, party):
        data = []
        net_params = NetParams()
        psi = None
        sender_obtain_result = False
        if party == 0:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8890
            net_params.local_port = 8891
            data = ["1", "2", "3"]
            sender_obtain_result = True
        else:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8891
            net_params.local_port = 8890
            data = ["2", "3"]
        psi = PSI(party, net_params, NetScheme.SOCKET, PSIScheme.ECDH_PSI)
        ret = psi.process(data, sender_obtain_result)
        if party == 0:
            self.assertEqual(ret, ["2", "3"])

    def run_kkrt_psi_process(self, party):
        data = []
        net_params = NetParams()
        psi = None
        sender_obtain_result = False
        if party == 0:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8890
            net_params.local_port = 8891
            data = ["1", "2", "3"]
            sender_obtain_result = True
        else:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8891
            net_params.local_port = 8890
            data = ["2", "3"]
        psi = PSI(party, net_params, NetScheme.SOCKET, PSIScheme.KKRT_PSI)
        ret = psi.process(data, sender_obtain_result)
        if party == 0:
            self.assertEqual(ret, ["2", "3"])

    def run_circuit_psi_process(self, party):
        keys = []
        features = [[]]
        net_params = NetParams()
        psi = None
        if party == 0:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8890
            net_params.local_port = 8891
            keys = ["1", "2", "3"]
            features = [[1, 2, 3]]
            sender_obtain_result = True
        else:
            net_params.remote_addr = "127.0.0.1"
            net_params.remote_port = 8891
            net_params.local_port = 8890
            keys = ["2", "3", "4"]
            features = [[5, 7, 9]]
        pjc = PJC(party, net_params, NetScheme.SOCKET, PJCScheme.CIRCUIT_PSI)
        ret = pjc.process(keys, features)
        print(party, ret)

    def test_ecdh_psi(self):

        processes = []

        for i in range(2):
            p = Process(target=self.run_ecdh_psi_process, args=(i,))
            p.start()
            processes.append(p)

        for p in processes:
            p.join()

    def test_kkrt_psi(self):

        processes = []

        for i in range(2):
            p = Process(target=self.run_kkrt_psi_process, args=(i,))
            p.start()
            processes.append(p)

        for p in processes:
            p.join()

    def test_cicuit_psi(self):

        processes = []

        for i in range(2):
            p = Process(target=self.run_circuit_psi_process, args=(i,))
            p.start()
            processes.append(p)

        for p in processes:
            p.join()


if __name__ == '__main__':
    unittest.main()
