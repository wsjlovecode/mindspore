mindspore.dataset.MnistDataset
===============================

.. py:class:: mindspore.dataset.MnistDataset(dataset_dir, usage=None, num_samples=None, num_parallel_workers=None, shuffle=None, sampler=None, num_shards=None, shard_id=None, cache=None)

    用于读取和解析MNIST数据集的源数据集文件。

    生成的数据集有两列: `[image, label]`。 `image` 列的数据类型为uint8。`label` 列的数据为uint32的标量。

    **参数：**

    - **dataset_dir** (str) - 包含数据集文件的根目录路径。
    - **usage** (str, 可选) - 指定数据集的子集，可取值为 `train`、`test` 或 `all`。使用 `train` 参数将会读取60,000个训练样本，`test` 将会读取10,000个测试样本，`all` 将会读取全部70,000个样本（默认值为None，即全部样本图片）。
    - **num_samples** (int, 可选) - 指定从数据集中读取的样本数（可以小于数据集总数，默认值为None,即全部样本图片)。
    - **num_parallel_workers** (int, 可选) - 指定读取数据的工作线程数（默认值None，即使用mindspore.dataset.config中配置的线程数）。
    - **shuffle** (bool, 可选) - 是否混洗数据集（默认为None，下表中会展示不同配置的预期行为）。
    - **sampler** (Sampler, 可选) - 指定从数据集中选取样本的采样器（默认为None，下表中会展示不同配置的预期行为）。
    - **num_shards** (int, 可选) - 分布式训练时，将数据集划分成指定的分片数（默认值None）。指定此参数后, `num_samples` 表示每个分片的最大样本数。
    - **shard_id** (int, 可选) - 分布式训练时，指定使用的分片ID号（默认值None）。只有当指定了 `num_shards` 时才能指定此参数。
    - **cache** (DatasetCache, 可选) - 单节点数据缓存，能够加快数据加载和处理的速度（默认值None，即不使用缓存加速）。

    **异常：**

    - **RuntimeError** - `dataset_dir` 路径下不包含数据文件。
    - **RuntimeError** - `num_parallel_workers` 超过系统最大线程数。
    - **RuntimeError** - 同时指定了 `sampler` 和 `shuffle` 参数。
    - **RuntimeError** - 同时指定了 `sampler` 和 `num_shards` 参数。
    - **RuntimeError** - 指定了 `num_shards` 参数，但是未指定 `shard_id` 参数。
    - **RuntimeError** - 指定了 `shard_id` 参数，但是未指定 `num_shards` 参数。
    - **ValueError** - `shard_id` 参数错误（小于0或者大于等于 `num_shards` ）。

    .. note:: 此数据集可以指定 `sampler` 参数，但 `sampler` 和 `shuffle` 是互斥的。下表展示了几种合法的输入参数及预期的行为。

    .. list-table:: 配置 `sampler` 和 `shuffle` 的不同组合得到的预期排序结果
       :widths: 25 25 50
       :header-rows: 1

       * - 参数 `sampler`
         - 参数 `shuffle`
         - 预期数据顺序
       * - None
         - None
         - 随机排列
       * - None
         - True
         - 随机排列
       * - None
         - False
         - 顺序排列
       * - 参数 `sampler`
         - None
         - 由 `sampler` 行为定义的顺序
       * - 参数 `sampler`
         - True
         - 不允许
       * - 参数 `sampler`
         - False
         - 不允许

    **样例：**

    >>> mnist_dataset_dir = "/path/to/mnist_dataset_directory"
    >>>
    >>> # 从MNIST数据集中随机读取3个样本
    >>> dataset = ds.MnistDataset(dataset_dir=mnist_dataset_dir, num_samples=3)
    >>>
    >>> # 提示：在MNIST数据集生成的数据集对象中，每一次迭代得到的数据行都有"image"和"label"两个键

    **关于MNIST数据集:**
    
    MNIST手写数字数据集是NIST数据集的子集，共有60,000个训练样本和10,000个测试样本。

    以下为原始MNIST数据集结构，您可以将数据集解压成如下的文件结构，并通过MindSpore的API进行读取。

    .. code-block::

        . 
        └── mnist_dataset_dir
            ├── t10k-images-idx3-ubyte
            ├── t10k-labels-idx1-ubyte
            ├── train-images-idx3-ubyte
            └── train-labels-idx1-ubyte

    **引用：**

    .. code-block::

        @article{lecun2010mnist,
        title        = {MNIST handwritten digit database},
        author       = {LeCun, Yann and Cortes, Corinna and Burges, CJ},
        journal      = {ATT Labs [Online]},
        volume       = {2},
        year         = {2010},
        howpublished = {http://yann.lecun.com/exdb/mnist}
        }

    .. include:: mindspore.dataset.Dataset.add_sampler.rst

    .. include:: mindspore.dataset.Dataset.rst

    .. include:: mindspore.dataset.Dataset.use_sampler.rst

    .. include:: mindspore.dataset.Dataset.zip.rst