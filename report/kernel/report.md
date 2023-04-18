# Rapport de laboratoir CSEL

## Modules noyaux

### Exercice 1 - Génération d'un module noyau
Afin de tester la génération et l'insertion d'un module noyau, nous avons
développé un module qui affiche un message dans le debug lorsqu'il
est chargé. Le code source est disponible le suivant:

```c
// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters

static char* text = "This is the best module ever made!";
module_param(text, charp, 0664);
static int  elements = 1;
module_param(elements, int, 0);

static int __init best_init(void)
{
	pr_info ("Linux module 01 best loaded\n");
	pr_debug ("  text: %s\n  elements: %d\n", text, elements);
	return 0;
}

static void __exit best_exit(void)
{
	pr_info ("Linux module best unloaded\n");
}

module_init (best_init);
module_exit (best_exit);

MODULE_AUTHOR ("Samy Francelet <samy.francelet@ik.me>");
MODULE_DESCRIPTION ("Module best");
MODULE_LICENSE ("GPL");
```

Lors de son insertion, le module affiche le message suivant (obtenu avec dmesg):

```bash
[  678.337447] Linux module 01 best loaded
[  678.341360]   text: This is the best module ever made!
[  678.341360]   elements: 1
```

Comparaison des commandes lsmod et cat /proc/modules:

- lsmod
```bash
Module                  Size  Used by    Tainted: G  
mymodule               16384  0 
```

- cat /proc/modules
```bash
mymodule 16384 0 - Live 0xffff80000122b000 (O)
```

Les deux commandes affichent le nom du module, sa taille et le nombre de
processus qui l'utilisent. La commande lsmod affiche également si le module
est marqué comme "tainted" (c'est-à-dire si le module a été compilé avec
une version différente du noyau). La commande cat /proc/modules affiche
également l'état du module (Live, Loading, Unloading, etc.).

Lors de son retrait, le module affiche le message suivant (obtenu avec dmesg):

```bash
[  742.206592] Linux module best unloaded
```

Pour permettre l'installation du module avec la commande modprobe,
il faut ajouter la ligne suivante dans le Makefile:
```make
install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
```
indiquant que le module doit être installé dans le répertoire $(MODPATH) lors
de l'installation.


### Exercice 2 - Paramètres de module

Dans cet exercice, il fallait créer un module pouvant recevoir des paramètres
de la ligne de commande.

Pour cela, nous avons utilisé la fonction module_param() qui permet de
définir des paramètres de la ligne de commande. Cette fonction prend en
paramètre le nom du paramètre, son type, et les permissions d'accès.

Nous avons défini deux paramètres: un paramètre de type chaîne de caractères
et un paramètre de type entier. Le premier paramètre est initialisé à la
chaîne de caractères "This is the best module ever made!" et le second
paramètre est initialisé à 1.

Pour tester le module avec des paramètres différents, nous avons utilisé
la commande suivante:
```bash
modprobe mymodule.ko text="This is the worst module ever made!" elements=1234
```

Donnant le résultat suivant:
```bash
[ 2097.961276] Linux module 01 best loaded
[ 2097.965210]   text: This is the worst module ever made!
[ 2097.965210]   elements: 1234
```

### Exercice 3 - cat /proc/sys/kernel/printk

La command cat /proc/sys/kernel/printk affiche le résultat suivant:
```bash
7       4       1       7
```
Le resultat montre les niveaux de priorité minimum des messages affichés
par le noyau (dans l'ordre: *current*, *default*, *minimum*, 
boot-time-default*). Les niveaux de priorité sont les suivants:
- 0: Emergency
- 1: Alert
- 2: Critical
- 3: Error
- 4: Warning
- 5: Notice
- 6: Info
- 7: Debug

### Exercice 4 - Gestion de la mémoire

Dans cet exercice, il fallait allouer dynamiquement de la mémoire pour
spécifier le nombre d'éléments à créer, et initialiser ces éléments avec
un texte passé en paramètre. Chaque élément doit posséder un identifiant
unique. Les éléments doivent être détruits lors de la suppression du module.

Pour cela, nous avons utilisé la fonction kzalloc() qui permet d'allouer
de la mémoire et de l'initialiser à 0. Cette fonction prend en paramètre
la taille de la mémoire à allouer et la priorité d'allocation de la mémoire.

Résultat de l'installation/désinstallation du module:
```bash
[ 3188.026403] Linux module 01 best loaded
[ 3188.030311]   text: Hello
[ 3188.030311]   elements: 12
[ 3196.895052] Element 0: Hello
[ 3196.897994] Element 1: Hello
[ 3196.900925] Element 2: Hello
[ 3196.903818] Element 3: Hello
[ 3196.906717] Element 4: Hello
[ 3196.909599] Element 5: Hello
[ 3196.912493] Element 6: Hello
[ 3196.915383] Element 7: Hello
[ 3196.918277] Element 8: Hello
[ 3196.921156] Element 9: Hello
[ 3196.924050] Element 10: Hello
[ 3196.927026] Element 11: Hello
[ 3196.929992] Number of elements: 12
[ 3196.933407] All elements deleted (12 out of 12)
[ 3196.937942] Linux module best unloaded
```

### Exercice 5 - Accès aux entrées/sorties

Le but de cet exercice était de créer un module qui peut accéder aux
entrées/sorties du système. Pour cela, nous avons utilisé la fonction
request_mem_region() qui permet de demander l'accès à une région mémoire
spécifique. Cette fonction prend en paramètre l'adresse de la région mémoire
et la taille de la région mémoire.

Ensuite si l'accès à la région mémoire est autorisé, nous avons utilisé
la fonction ioremap() qui permet de mapper une région mémoire physique
dans l'espace d'adressage virtuel. Cette fonction prend en paramètre
l'adresse de la région mémoire physique et la taille de la
région mémoire (typiquement 1 page, soit 0x1000 octets).

Avec la fonction iorea(), nous pouvons accéder à la mémoire physique
à l'adresse virtuelle retournée par la fonction ioremap(). Ce qui nous
permet de lire, par exemple, la valeur de température du processeur.

### Exercice 6 - Threads du noyau

Afin de créer un thread du noyau, nous avons utilisé la fonction
kthread_run() qui permet de créer et de lancer un thread du noyau.
Cette fonction prend en paramètre la fonction à exécuter, les paramètres
de la fonction, et le nom du thread.

Dans la fonction à exécuter, nous avons utilisé la fonction
kthread_should_stop() dans une boucle qui permet de vérifier si le thread doit être
arrêté. Cette fonction retourne 1 si le thread doit être arrêté, 0 sinon.
Cela diffère du fonctionnement des threads utilisateurs qui utilisent simplement
des boucles infinies.

Au retrait du module, nous avons utilisé la fonction kthread_stop() qui
permet d'arrêter un thread du noyau.

### Exercice 7 - Mise en sommeil

Dans cet exercice, il fallait créer deux threads du noyau qui s'exécutent
en parallèle. Le premier thread doit attendre une notification de réveil
pour s'exécuter. Le second thread transmet la notification de réveil au
premier thread via une waitqueue.

Pour cela, nous avons utilisé la fonction wait_event_interruptible() qui
permet de mettre un thread en attente d'une notification. Cette fonction
prend en paramètre une waitqueue et une fonction de test.
Cette fonction retourne 0 si la notification a été reçue, -ERESTARTSYS
si le thread a été interrompu.

### Exercice 8 - Gestion des interruptions

Dans cet exercice, il fallait générer une interruption
quand un bouton est appuyé.

Pour cela, nous avons d'abord utilisé la fonction gpio_request() qui
permet de demander l'accès à un port GPIO. Cette fonction prend en
paramètre le numéro du port GPIO et le nom du port GPIO.

Une fois le port GPIO accédé, nous avons utilisé la fonction
request_irq() qui permet de demander l'accès à une interruption. Cette
fonction prend en paramètre le numéro de l'interruption, la fonction
à exécuter lors de l'interruption, le type d'interruption, le nom de
l'interruption, et un pointeur vers des données.

Dans la fonction à exécuter (de type irq_handler_t), nous retournons
IRQ_HANDLED pour indiquer que l'interruption a été traitée.
Dans le cas d'une interruption différée, nous retournons IRQ_WAKE_THREAD
(il aurait fallu utiliser la fonction request_threaded_irq() pour cela).

## Pilotes de périphériques

### Exercice 1 - Pilotes orientés mémoire

Dans cet exercice, il fallait réaliser un pilote orienté mémoire permettant
de mapper en espace utilisateur les registres du microprocesseur en
utilisant /dev/mem.
Ce pilote permet de lire le Chip-ID.

Pour ce faire, nous avons procédé ainsi:
- Ouvrir le fichier /dev/mem avec la fonction open()
- Mapper la mémoire physique du Chip-ID dans l'espace d'adressage virtuel
  avec la fonction mmap(), avec les paramètres suivants:
  - NULL: laisse le kernel choisir l'adresse virtuelle
  - pz: une taille de page
  - PROT_READ | PROT_WRITE: autorise la lecture et l'écriture
  - MAP_SHARED: partage la mémoire avec d'autres processus
  - fd: le descripteur de fichier du fichier /dev/mem précédemment ouvert
  - l'offset du Chip-ID dans le fichier /dev/mem (soit dev_addr - (dev_addr % pz))
- Lu les registres du Chip-ID à l'adresse virtuelle retournée par mmap()
- Dé-mapper la mémoire avec la fonction munmap()
- Fermer le fichier /dev/mem avec la fonction close()

### Pilotes orientés caractères
#### Exercice 2

Dans cet exercice, il fallait réaliser un pilote orienté caractère
capable de stocker dans une variable globale au module les données
reçues par l'opération write et de les restituer par l'opération read.
Pour tester le module, nous utiliserons les commandes echo et cat.