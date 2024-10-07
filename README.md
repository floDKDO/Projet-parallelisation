# Rapport HALM Florian JIANG Chuyang


## Difficultées rencontrées : 
On a voulu implémenter la version de l'algorithme de fermeture transitive parallèle de l'article mais cela a été très compliqué. En effet, on a essayé par tous les moyens possibles d'implémenter la partie d'envoi des cases modifiées de la matrice n'appartenant pas au processus courant mais on a jamais réussi.

## Version implémentée :
- Modifications dans la fonction warshall :

On a supprimmé la boucle d'initialisation de la matrice c car elle est inutile dans notre cas : chaque processus réalise les calculs directement sur la matrice a, les résultats seront envoyés dans la matrice c du processus 0.

Chaque processus va recevoir une bande de la matrice a via MPI_Scatter, bande de la taille n*n/size éléments. Bien entendu, il faut que n soit divisible par size.

Ensuite, on a crée une région parallèle qui créée un partage de travail entre threads pour la boucle for avec l'itérateur k, l'itérateur i de la première boucle imbriquée étant mis en private. 

Dans la boucle imbriquée avec l'itérateur i, on a réduit la valeur maximale de i à n/size car chaque processus aura uniquement une bande de n/size lignes.

Ensuite, on a ajouté deux tests permettant de ne pas réaliser d'opérations inutiles pour gagner du temps : test si a[i][k] == 1 et test si a[k][j] == 1.

De même, la modification de a[i][j] qui se faisait avec un OU logique et un ET logique dans la version séquentielle a été remplacé par une assignation à 1 : en effet, les deux tests précédents nous assurent que a[i][k] et a[k][j] valent 1, donc on a : a[i][j] = a[i][j] || (1 && 1) <=> a[i][j] = a[i][j] || 1 <=> a[i][j] = 1 car quelque chose OU 1 vaut toujours 1. Cette modification fait gagner du temps car on enlève le besoin de regarder d'autres cases de la matrice a à chaque modification de a[i][j].

A la fin de l'algorithme, le processus de rang 0 va récupérer les bandes de matrices calculées par les autres processus pour former la matrice c stockant le résultat de la fonction warshall via MPI_Gather.

- Modifications dans le main : 

Initialisation et terminaison de MPI, affichages réalisés uniquement par le processus de rang 0.


## Performances

### <ins>Performances obtenues sur la plateforme OpenStack : </ins>


|     |     | hosts | processes | threads/proc |        1000 |        3000 | 10000        |
|-----|-----|-------|-----------|--------------|-------------|-------------|--------------|
| (a) | seq |     1 |         1 |            1 | 0.73        | 19.90       | 733.17       |
| (b) | par |     4 |         4 |            4 | 0.124       | 1.665       | 44.933       |
| (c) | par |     4 |         8 |            4 | 0.154       | 1.006       | 26.024       |
| (d) | par |     8 |         8 |            4 | 0.140       | 1.074       | 24.847       |

<ins>Informations : </ins>
- le temps indiqué dans les colonnes 1000-3000-10000 vaut Init+Compute.
- (a) : exécution séquentielle (1 host, 1 processus), pas d'OpenMP donc 1 thread. 
- (b) : exécution parallèle avec `-n 4` avec 4 hosts dans `hostfile`, chaque host a `slots=1` donc on a 1 processus par host, chaque processus utilisant 4 threads pour openmp.
- (c) : exécution parallèle avec `-n 8` avec 4 hosts dans `hostfile`, chaque host a `slots=2` donc on a 2 processus par host, chaque processus utilisant 4 threads pour openmp. Les résultats sont mauvais.
- (d) : exécution parallèle avec `-n 8` avec 8 hosts dans `hostfile`, chaque host a `slots=1` donc on a 1 processus par host, chaque processus utilisant 4 threads pour openmp.


<br><ins>Pour le cas (b) : </ins>

make run-par-1000
<br> *Accélération* : 0.73/0.124 = **5.88**
<br> *Efficacité* : 5.88/4 = **1.47**

make run-par-3000
<br> *Accélération* : 19.90/1.665 = **11.95**
<br> *Efficacité* : 11.95/4 = **2.98**

make run-par-10000
<br> *Accélération* : 733.17/44.933 = **16.31**
<br> *Efficacité* : 16.31/4 = **4.07**


<br><ins>Pour le cas (c) : </ins>

make run-par-1000
<br> *Accélération* : 0.73/0.154 = **4.74**
<br> *Efficacité* : 4.74/8 = **0.59**

make run-par-3000
<br> *Accélération* : 19.90/1.006 = **19.78**
<br> *Efficacité* : 19.78/8 = **2.47**

make run-par-10000
<br> *Accélération* : 733.17/26.024 = **28.17**
<br> *Efficacité* : 28.17/8 = **3.52**


<br><ins>Pour le cas (d) : </ins>

make run-par-1000
<br> *Accélération* : 0.73/0.140 = **5.21**
<br> *Efficacité* : 5.21/8 = **0.65**

make run-par-3000
<br> *Accélération* : 19.90/1.074 = **18.52**
<br> *Efficacité* : 18.52/8 = **2.31**

make run-par-10000
<br> *Accélération* : 733.17/24.847 = **29.50**
<br> *Efficacité* : 29.50/8 = **3.68**

###################################################################################

### <br><ins>Performance sur un ordinateur 4 coeurs (un seul host étant le localhost) : </ins>


|     | hosts | processes |        1000 |        3000 | 10000        |
|-----|-------|-----------|-------------|-------------|--------------|
| seq |     1 |         1 |    1.508    |    53.3     | 1312.75      |
| par |     1 |         4 |    0.206    |    3.93     | 71.56        |
| par |     1 |         8 |    0.226    |    4.247    | 74.276       |


<br><ins>Pour le cas taille du jeu de données = 1000 : </ins>

make run-seq-1000 Init : 0.038505s, Compute : 1.470949s = 1.508s\
<br> make run-par-1000
<br> => (4 processus) Init : 0.086602s, Compute : 0.122350s = 0.206s
<br> *Accélération* : 1.508/0.206 = **7.32**
<br> *Efficacité* : 7.32/4 = **1.83**
<br> => (8 processus) Init : 0.106148s, Compute : 0.120522s = 0.226s
<br> *Accélération* : 1.508/0.226 = **6.67**
<br> *Efficacité* : 6.67/8 = **0.83**

<br><ins>Pour le cas taille du jeu de données = 3000 : </ins>

make run-seq-3000 => Init : 0.499085s, Compute : 52.853353s = 53.3s\
<br> make run-par-3000
<br> => (4 processus) Init : 0.921538s, Compute : 3.011911s = 3.93s
<br> *Accélération* : 53.3/3.93 = **13.56**
<br> *Efficacité* : 13.56/4 = **3.39**
<br> => (8 processus) Init : 1.127158s, Compute : 3.127511s = 4.247s
<br> *Accélération* : 53.3/4.247 = **12.55**
<br> *Efficacité* : 12.55/8 = **1.56**

<br><ins>Pour le cas taille du jeu de données = 10000 : </ins>

make run-seq-10000 => Init : 3.410350s, Compute : 1309.345523s = 1312.75s\
<br> make run-par-10000
<br> => (4 processus) Init : 6.626943s, Compute : 64.948357 = 71.56s
<br> *Accélération* : 1312.75/71.56 = **18.34**
<br> *Efficacité* : 18.34/4 = **4.58**
<br> => (8 processus) Init : 10.448615s, Compute : 63.828348s = 74.276s
<br> *Accélération* : 1312.75/74.276 = **17.67**
<br> *Efficacité* : 17.67/8 = **2.2**

###################################################################################

### <br> <ins> Performance sur un ordinateur 6 coeurs (un seul host étant le localhost) : </ins>


|     | hosts | processes |        1000 |        3000 | 10000        |
|-----|-------|-----------|-------------|-------------|--------------|
| seq |     1 |         1 |    0.742    |    20.3     | 691.655      |
| par |     1 |         4 |    0.124    |    1.074    | 29.03        |
| par |     1 |         8 |    0.186    |    1.203    | 29.96        |


<br><ins>Pour le cas taille du jeu de données = 1000 : </ins>

make run-seq-1000 Init : 0.032221s, Compute : 0.716010s = 0.742s\
<br> make run-par-1000
<br> => (4 processus) Init : 0.041168s, Compute : 0.083185s = 0.124s
<br> *Accélération* : 0.742/0.124 = **5.98**
<br> *Efficacité* : 5.98/4 = **1.49**
<br> => (8 processus) Init : 0.056103s, Compute : 0.136120s = 0.186s
<br> *Accélération* : 0.742/0.186 = **3.98**
<br> *Efficacité* : 3.98/8 = **0.49**

<br><ins>Pour le cas taille du jeu de données = 3000 : </ins>

make run-seq-3000 => Init : 0.245060s, Compute : 20.067165s = 20.3s\
<br> make run-par-3000
<br> => (4 processus) Init : 0.374826s, Compute : 0.700827s = 1.074s
<br> *Accélération* : 20.3/1.074 = **18.9**
<br> *Efficacité* : 18.9/4 = **4.7**
<br> => (8 processus) Init : 0.370494s, Compute : 0.833103s = 1.203s
<br> *Accélération* : 20.3/1.203 = **16.8**
<br> *Efficacité* : 16.8/8 = **2.1**

<br><ins>Pour le cas taille du jeu de données = 10000 : </ins>

make run-seq-10000 => Init : 2.665047s, Compute : 688.990858s = 691.655s\
<br> make run-par-10000
<br> => (4 processus)Init : 3.892398s, Compute : 25.141017s = 29.03s
<br> *Accélération* : 691.655/29.03 = **23.82**
<br> *Efficacité* : 23.82/4 = **5.9**
<br> => (8 processus) Init : 4.585268s, Compute : 25.379239s = 29.96s
<br> *Accélération* : 691.655/29.96 = **23.08**
<br> *Efficacité* : 23.08/8 = **2.88**
