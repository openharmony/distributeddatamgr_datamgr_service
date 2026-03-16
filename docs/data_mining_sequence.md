# Data Mining 时序图

## 多 Pipeline 说明

- DDMS 在 source 回调时会带上 `pipelineName` 和 `sourceName`，只把本次命中的那条 pipeline 送到 ETL SA。
- ETL SA 内部按 `pipelineName` 维护 `pipelines_[pipelineName]` 和 `runtimes_[pipelineName]`，`Dispatch` 不会广播执行所有 pipeline。
- 如果多个 pipeline 同时订阅了同一个 source/topic，那么会出现“各自独立触发、各自独立 dispatch”的效果；这是多策略并存带来的结果，不是 ETL SA 误执行了全量 pipeline。

## 多 Pipeline 共享同一 Source/Topic

```mermaid
sequenceDiagram
    participant Dev as Pipeline开发者
    participant DDMS as DDMS DataMining Feature
    participant ProxyA as SourceProxy(A)
    participant ProxyB as SourceProxy(B)
    participant Source as Source/lib或SourceClient
    participant ETL as ETL SA(1311)
    participant RuntimeA as PipelineRuntime(A)
    participant RuntimeB as PipelineRuntime(B)

    Dev->>DDMS: 部署 pipelineA.json 和 pipelineB.json
    DDMS->>ProxyA: 为 pipeline A 建立 Subscribe(topic=T)
    DDMS->>ProxyB: 为 pipeline B 建立 Subscribe(topic=T)
    ProxyA->>Source: Subscribe(topic=T, notifierA)
    ProxyB->>Source: Subscribe(topic=T, notifierB)

    Source-->>ProxyA: 回调数据给 notifierA
    ProxyA-->>DDMS: OnSourceOutput(pipelineName=A, sourceName=S, data)
    DDMS->>ETL: Dispatch(pipelineName=A, fromNode=S, value)
    ETL->>RuntimeA: 仅执行 runtime[A]

    Source-->>ProxyB: 回调数据给 notifierB
    ProxyB-->>DDMS: OnSourceOutput(pipelineName=B, sourceName=S, data)
    DDMS->>ETL: Dispatch(pipelineName=B, fromNode=S, value)
    ETL->>RuntimeB: 仅执行 runtime[B]

    Note over DDMS,ETL: 如果只收到 A 的回调，只会执行 pipeline A；<br/>只有 B 自己也收到回调时，才会再次执行 pipeline B。
```

## 订阅型 Pipeline

```mermaid
sequenceDiagram
    participant Dev as Pipeline开发者
    participant DDMS as DDMS DataMining Feature
    participant Proxy as SourceProxy
    participant Source as Source/lib或SourceClient
    participant ETL as ETL SA(1311)
    participant Runtime as PipelineRuntime
    participant Op as Operator
    participant Sink as Sink

    Dev->>DDMS: 部署 pipeline.json / plugin.json
    DDMS->>DDMS: OnInitialize 读取配置
    DDMS->>DDMS: 解析 subscriptions / tree
    DDMS->>Proxy: 构造 sourceProxy(sourceName, endpoint)
    DDMS->>Proxy: StartPipeline -> Subscribe(topic)
    Proxy->>Source: Subscribe(topic, context, notifier)
    Source-->>Proxy: 数据捐赠/事件回调
    Proxy-->>DDMS: notifier.Notify(context, topic, data)
    DDMS->>ETL: LoadSystemAbility(1311)
    DDMS->>ETL: RegisterPipeline(pipelineName=A) / RegisterPlugin(...)
    DDMS->>ETL: Dispatch(pipelineName=A, fromNode=source, value)
    ETL->>Runtime: 仅获取 runtime[A]，不存在时按 pipeline A 构图
    Runtime->>Op: Process(data)
    Op-->>Runtime: notifier.Notify(nextData)
    Runtime->>Sink: Save(result)
```

## 定时型 Pipeline

```mermaid
sequenceDiagram
    participant Dev as Pipeline开发者
    participant DDMS as DDMS DataMining Feature
    participant Timer as Executor定时任务
    participant Proxy as SourceProxy
    participant Source as Source/lib或SourceClient
    participant ETL as ETL SA(1311)
    participant Runtime as PipelineRuntime
    participant Op as Operator
    participant Sink as Sink

    Dev->>DDMS: 部署 pipeline.json / plugin.json
    DDMS->>DDMS: OnInitialize 读取 timers / tree
    DDMS->>Proxy: 构造 sourceProxy(sourceName, endpoint)
    DDMS->>Timer: ScheduleTimerTask(interval)
    Timer-->>DDMS: 定时回调触发
    DDMS->>Proxy: Trigger(context, notifier)
    Proxy->>Source: Trigger(context, notifier)
    Source-->>Proxy: notifier.Notify(context, topic, data)
    Proxy-->>DDMS: source 输出回调
    DDMS->>ETL: LoadSystemAbility(1311)
    DDMS->>ETL: RegisterPipeline(pipelineName=B) / RegisterPlugin(...)
    DDMS->>ETL: Dispatch(pipelineName=B, fromNode=source, value)
    ETL->>Runtime: 仅复用/创建 runtime[B]
    Runtime->>Op: Process(data)
    Op-->>Runtime: notifier.Notify(nextData)
    Runtime->>Sink: Save(result)
    DDMS->>Timer: 回调尾部重新续约下一次定时任务
```
