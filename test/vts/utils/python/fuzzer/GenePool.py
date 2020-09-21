#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import random

REPLICATION_COUNT_IF_NEW_COVERAGE_IS_SEEN = 5
REPLICATION_PARAM_IF_NO_COVERAGE_IS_SEEN = 10


def CreateGenePool(count, generator, fuzzer, **kwargs):
  """Creates a gene pool, a set of test input data.

  Args:
    count: integer, the size of the pool.
    generator: function pointer, which can generate the data.
    fuzzer: function pointer, which can mutate the data.
    **kwargs: the args to the generator function pointer.

  Returns:
    a list of generated data.
  """
  genes = []
  for index in range(count):
    gene = generator(**kwargs)
    genes.append(fuzzer(gene))
  return genes


class Evolution(object):
  """Evolution class

  Attributes:
    _coverages_database: a list of coverage entities seen previously.
    _alpha: replication count if new coverage is seen.
    _beta: replication parameter if no coverage is seen.
  """

  def __init__(self, alpha=REPLICATION_COUNT_IF_NEW_COVERAGE_IS_SEEN,
               beta=REPLICATION_PARAM_IF_NO_COVERAGE_IS_SEEN):
    self._coverages_database = []
    self._alpha = alpha
    self._beta = beta

  def _IsNewCoverage(self, coverage, add=False):
    """Returns True iff the 'coverage' is new.

    Args:
      coverage: int, a coverage entity
      add: boolean, true to add coverage to the db if it's new.

    Returns:
      True if new, False otherwise
    """
    is_new_coverage = False
    new_coverage_entities_to_add = []
    for entity in coverage:
      if entity not in self._coverages_database:
        is_new_coverage = True
        if add:
          new_coverage_entities_to_add.append(entity)
        else:
          return True
    if add:
      self._coverages_database.extend(new_coverage_entities_to_add)
    return is_new_coverage

  def Evolve(self, genes, fuzzer, coverages=None):
    """Evolves a gene pool.

    Args:
      genes: a list of input data.
      fuzzer: function pointer, which can mutate the data.
      coverages: a list of the coverage data where coverage data is a list which
        contains IDs of the covered entities (e.g., basic blocks).
  
    Returns:
      a list of evolved data.
    """
    new_genes = []
    if not coverages:
      for gene in genes:
        # TODO: consider cross over
        new_genes.append(fuzzer(gene))
    else:
      for gene, coverage in zip(genes, coverages):
        if self._IsNewCoverage(coverage, add=True):
          for _ in range(self._alpha):
            new_genes.append(fuzzer(gene))
        elif random.randint(0, self._beta) == 1:
          new_genes.append(fuzzer(gene))
    return new_genes
