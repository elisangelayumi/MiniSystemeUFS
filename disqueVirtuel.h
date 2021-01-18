/**
 * \file disqueVirtuel.h
 * \brief Gestion d'un disque virtuel.
 * \author IFT-2001
 * \version 0.1
 * \date nov-dec 2020
 *
 *  Travail pratique numéro 3
 *
 */

#include "block.h"

#ifndef _DISQUEVIRTUEL__H
#define _DISQUEVIRTUEL__H

namespace TP3
{

#define N_INODE_ON_DISK   20	// nombre maximal d'i-nodes (donc de fichiers) sur votre disque
#define N_BLOCK_ON_DISK 128	// nombre de blocs sur le disque au complet
#define FREE_BLOCK_BITMAP 2	// numero du bloc contenant le bitmap des block libres
#define FREE_INODE_BITMAP 3	// numero du bloc contenant le bitmap des i-nodes libres
#define BASE_BLOCK_INODE  4     // bloc de depart ou les i-nodes sont stockes sur disque
#define ROOT_INODE        1     // numero du i-node correspondant au repertoire racine

class DisqueVirtuel {
public:
	DisqueVirtuel();
	~DisqueVirtuel();

	// Méthodes demandées
	int bd_FormatDisk();
	std::string bd_ls(const std::string& p_DirLocation);
	int bd_mkdir(const std::string& p_DirName);
	int bd_create(const std::string& p_FileName);
	int bd_rm(const std::string& p_Filename);

	// Vous pouvez ajouter ici d'autres méthodes publiques
	void splitDerniereNom(const std::string cheminFichier, std::string& path, std::string& nomFichier);
    void splitPremierRepertoire(const std::string path, std::string& cherche, std::string& restant);
    void creerRepertoireVide(size_t nbInodeParent, size_t nbiNodeLibre, size_t nbBlocLibre);
    int trouveBlocLibre();
    int trouveINodeLibre();
    int trouveNbINode(const std::string path);
    bool repertoireExiste(const std::string nomPath);
    bool nomExiste(const std::string nom);
    void afficherMessageBloc(bool commande, size_t nbBloc);
    void afficherMessageInode(bool commande, size_t nbINode);
    int calculNombreLiens(size_t nbInode);
    void miseAJourInodeParent(size_t nbInode);
    void miseAJourBitmapINode(size_t nbInode, bool statut);
    void miseAJourBitmapBloc(size_t nbBloc, bool statut);
    void libererInode(size_t nbInode);



private:
	// Il est interdit de modifier ce modèle d'implémentation (i.e le type de m_blockDisque)!
    std::vector<Block> m_blockDisque; // Un vecteur de blocs représentant le disque virtuel

    // Vous pouvez ajouter ici des méthodes privées
};

}//Fin du namespace

#endif
