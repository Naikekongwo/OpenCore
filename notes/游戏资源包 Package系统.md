# PackageSystem


## Package 结构

Package分为两种，一种是携带了packagemanifest的最终发布用资源包（其魔数为OCCP，OpenCore Combination Package），另一种为外置packagemanifest的中间资源包（魔数为DATA）

### OCCP 包 : OpenCore Combination Package 包

```mermaid
graph LR

subgraph data文件夹

    subgraph OCCP资源包 aozora_00.ocdata
        A[4 Bytes <br>OCMP]
        B[N Bytes <br>Manifest]
        C[M Bytes <br>连续储存的资源文件]
        D[4 Bytes <br>OFFSET = N]

        A-->B-->C-->D
    end

end
```

携带这个魔数的包需要经过引擎的提取，将清单文件提取出来后，才方便使用。

### DATA 包 : OpenCore Database 包


```mermaid
graph LR

subgraph data文件夹

    subgraph DATA资源包 aozo_00.ocdata
        A[4 Bytes <br>DATA]
        C[M Bytes <br>连续储存的资源文件]
        A-->C
    end

    D[aozo_packagemanifest.txt]

end
```

携带这个魔数的包是已经经过提取、可以直接使用的包