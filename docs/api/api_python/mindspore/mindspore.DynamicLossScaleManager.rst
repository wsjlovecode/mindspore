mindspore.DynamicLossScaleManager
==================================

.. py:class:: mindspore.DynamicLossScaleManager(init_loss_scale=16777216, scale_factor=2, scale_window=2000)

    动态调整梯度放大系数的管理器，继承自 :class:`mindspore.LossScaleManager` 。

    **参数：**

    - **init_loss_scale** (float) - 初始梯度放大系数。默认值：2**24。
    - **scale_factor** (int) - 放大/缩小倍数。默认值：2。
    - **scale_window** (int) - 无溢出时的连续正常step的最大数量。默认值：2000。

    **样例：**

    >>> from mindspore import Model, nn, DynamicLossScaleManager
    >>>
    >>> net = Net()
    >>> loss_scale_manager = DynamicLossScaleManager()
    >>> optim = nn.Momentum(params=net.trainable_params(), learning_rate=0.1, momentum=0.9)
    >>> model = Model(net, loss_scale_manager=loss_scale_manager, optimizer=optim)

    .. py:method:: get_drop_overflow_update()

        该值表示是否在发生溢出时放弃本轮参数更新。

        **返回：**

        bool，始终为True。

    .. py:method:: get_loss_scale()

        返回当前梯度放大系数。

        **返回：**

        float，梯度放大系数。

    .. py:method:: get_update_cell()

        返回用于更新梯度放大系数的 `Cell` 实例，:class:`mindspore.TrainOneStepWithLossScaleCell` 会调用该实例。

        **返回：**

        :class:`mindspore.DynamicLossScaleUpdateCell` 实例，用于更新梯度放大系数。

    .. py:method:: update_loss_scale(overflow)

        根据溢出状态更新梯度放大系数。如果发生溢出，减小梯度放大系数，否则增大梯度放大系数。

        **参数：**

        - **overflow** (bool) - 表示是否溢出。
