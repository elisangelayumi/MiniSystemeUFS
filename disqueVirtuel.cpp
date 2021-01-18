/**
 * \file disqueVirtuel.cpp
 * \brief Implémentation d'un disque virtuel.
 * \author Elisangela Yumi
 * \version 0.1
 * \date nov-dec 2020
 *
 *  Travail pratique numero 3 du cours Système d'Exploitation
 *
 */

#include "disqueVirtuel.h"
#include <iostream>
#include <string>
//vous pouvez inclure d'autres librairies si c'est necessaire
#include <sstream>
#include <iomanip>
#include <algorithm>


namespace TP3
{
    /**
    * \brief Constructeur avec parametres
    * \return Un objet DisqueVirtuel
    */
    DisqueVirtuel::DisqueVirtuel(){
        m_blockDisque.resize(N_BLOCK_ON_DISK);
    }

    /**
    * \brief Destructeur
    */
    DisqueVirtuel::~DisqueVirtuel(){

        for (unsigned int i = 0; i < m_blockDisque.size(); i++)
        {
           delete m_blockDisque[i].m_inode;
        }


        for (unsigned int i = 0; i < m_blockDisque.size(); i++)
        {
            for (int j=0; j < m_blockDisque[i].m_dirEntry.size(); j++){
                delete m_blockDisque[i].m_dirEntry[j];
            }
        }

        m_blockDisque.clear();
    }

    /**
    * \brief La méthode efface le disque virtuel et crée le système de fichier UFS
    * \return 1 pour succes ou 0 pour echec
    */
    int DisqueVirtuel::bd_FormatDisk(){

        // on efface le contenu du disque pour le formatage.
        m_blockDisque.clear();
        m_blockDisque.resize(N_BLOCK_ON_DISK);

        // on initialise les blocks 2 et 3.
        m_blockDisque.at(FREE_BLOCK_BITMAP) = Block(S_IFBL);
        m_blockDisque.at(FREE_INODE_BITMAP) = Block(S_IFIL);

        //initiliaser m_bitmap
        m_blockDisque.at(FREE_BLOCK_BITMAP).m_bitmap.resize(N_BLOCK_ON_DISK);
        m_blockDisque.at(FREE_INODE_BITMAP).m_bitmap.resize(N_INODE_ON_DISK);

        // les blocs 0-23 sont deja initiliase à false, alors non libres. On marque les autres comme libres (true)
        for (int i =24; i < N_BLOCK_ON_DISK; i++ ){
            m_blockDisque.at(FREE_BLOCK_BITMAP).m_bitmap.at(i) = true;
        }

        // on cree les INodes dans les blocks 4 a 23.
        for (int i = BASE_BLOCK_INODE; i <= 23 ; i++){
            m_blockDisque.at(i).m_type_donnees = S_IFIN;
            m_blockDisque.at(i).m_inode = new iNode(i - BASE_BLOCK_INODE,0, 0, 0, 0);
        }

        //  marque les iNodes comme libres (sauf INode 0 = false)
        for (int i=1; i < N_INODE_ON_DISK; i++){
            m_blockDisque.at(FREE_INODE_BITMAP).m_bitmap.at(i) = true;
        }

        // creer répertoire racine
        int blocLibre = trouveBlocLibre();
        int iNodeLibre = trouveINodeLibre();
        creerRepertoireVide(iNodeLibre, iNodeLibre, blocLibre);

        return 1;
    }

    /**
    * \brief liste tous les fichiers et répertoires présents dans le répertoire pDirLocation
    * \return un string qui contient tous les fichiers et répertoires présents dans le répertoire pDirLocation
    */
    std::string DisqueVirtuel::bd_ls(const std::string& p_DirLocation){

        std::ostringstream os;
        if (!repertoireExiste(p_DirLocation)) {
            os << "Le répertoire " << p_DirLocation << " n'existe pas!" << std::endl ;
            return os.str();
        }

        std::string path, pathCherche;
        splitDerniereNom(p_DirLocation, path, pathCherche );
        int monInode;

        if (path == "/" && pathCherche == ""){ //si on veut faire un ls de la racine
            monInode = ROOT_INODE;
        }
        else{
            monInode = trouveNbINode(pathCherche);
        }

        int monBloc = m_blockDisque.at(monInode + BASE_BLOCK_INODE).m_inode->st_block;
        int n = 16; //l'espacement entre le colonnes

        os << p_DirLocation << std::endl;
        for (int i = 0; i < m_blockDisque.at(monBloc).m_dirEntry.size(); i++) {
            int j = m_blockDisque.at(monBloc).m_dirEntry.at(i)->m_iNode;
            if (m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_mode == S_IFDIR) {
                os << "d" << std::setw(n + 1) << m_blockDisque.at(monBloc).m_dirEntry.at(i)->m_filename
                   << "  Size:" << std::setw(n / 2) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_size
                   << "  inode:" << std::setw(n / 2.5) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_ino
                   << "  nlink:" << std::setw(n / 2.5) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_nlink
                   << std::endl;
            } else {
                os << "-" << std::setw(n + 1) << m_blockDisque.at(monBloc).m_dirEntry.at(i)->m_filename
                   << "  Size:" << std::setw(n / 2) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_size
                   << "  inode:" << std::setw(n / 2.5) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_ino
                   << "  nlink:" << std::setw(n / 2.5) << m_blockDisque.at(j + BASE_BLOCK_INODE).m_inode->st_nlink
                   << std::endl;
            }
        }

        return os.str();
    }

    /**
    * \brief cree des répertoires
    * \return 1 pour succes ou 0 pour echec
    */
    int DisqueVirtuel::bd_mkdir(const std::string& p_DirName){

        std::string path, nomDirACreer;
        splitDerniereNom(p_DirName, path, nomDirACreer );
        if (repertoireExiste(path)){  //d'abord on check si le chemin existe
            if (nomExiste(p_DirName) == false){ // on check si le nom est valide c.a.d si n'existe pas d'autre répertoire/fichier avec ce nom

                //on cherche le bloc et inode libre
                int blocLibre = trouveBlocLibre();
                int iNodeLibre = trouveINodeLibre();
                //on cherche le bloc et inode où sera cree le DIR.
                int iNodeParent = trouveNbINode(path); //  "path" contient deja le chemin où on doit creer le répertoire, on cherche ce inode.
                int blocParent = m_blockDisque.at(iNodeParent + BASE_BLOCK_INODE).m_inode->st_block;

                //cree le répertoire
                dirEntry *pRep = new dirEntry(iNodeLibre, nomDirACreer );

                //on ajoute le répertoire dans le bloc parent
                m_blockDisque.at(blocParent).m_dirEntry.push_back(pRep);

                //on cree les répertoires vide et on met a jour les infos de l’inode Libre
                creerRepertoireVide( iNodeParent, iNodeLibre, blocLibre );

                //mise a jour des info de l’inode  parent
                miseAJourInodeParent(iNodeParent);

                return 1;
            }
            std::cout << "mkdir: Le nom " << nomDirACreer  << " existe déjà dans le répertoire " << path << std::endl; ;
            return 0;
        }
        std::cout << "mkdir: Le répertoire " << path  << " n'existe pas!" << std::endl;
        return 0;
    }

    /**
    * \brief cree des fichiers
    * \return 1 pour succes ou 0 pour echec
    */
    int DisqueVirtuel::bd_create(const std::string& p_FileName){

        std::string path, nomFichier;
        splitDerniereNom(p_FileName, path, nomFichier );

        if(repertoireExiste(path)){
            if (nomExiste(p_FileName) == false){

                int iNodeLibre = trouveINodeLibre();

                //on cherche le bloc et inode parent.
                int iNodeParent = trouveNbINode(path); //"path" contient deja le chemin où on doit creer le répertoire, on cherche ce inode.
                int blocParent = m_blockDisque.at(iNodeParent + BASE_BLOCK_INODE).m_inode->st_block;

                //cree le fichier
                dirEntry *pFichier = new dirEntry(iNodeLibre, nomFichier );

                //on ajoute le fichier dans le bloc parent
                m_blockDisque.at(blocParent).m_dirEntry.push_back(pFichier);

                // on met a jour les attributs de l'inode libre
                m_blockDisque.at(iNodeLibre + BASE_BLOCK_INODE).m_inode->st_mode = S_IFREG;
                m_blockDisque.at(iNodeLibre + BASE_BLOCK_INODE).m_inode->st_nlink = calculNombreLiens(iNodeLibre);
                m_blockDisque.at(iNodeLibre + BASE_BLOCK_INODE).m_inode->st_size = 0; //puisqu'on cree de fichiers vide -> toujours zero

                // met a jour les bitmap
                miseAJourBitmapINode(iNodeLibre, false);

                //mise a jour des info de l’inode  parent
                miseAJourInodeParent(iNodeParent);

                return 1;
            }
            std::cout << "create: Le nom " << nomFichier  << " existe déjà dans le répertoire " << path << std::endl; ;
            return 0;
        }
        std::cout << "create: Le répertoire " << path  << " n'existe pas!" << std::endl;
        return 0;
    }

    /**
    * \brief liste tous les fichiers et répertoires présents dans le répertoire pDirLocation
    * \return un string qui contient tous les fichiers et répertoires présents dans le répertoire pDirLocation
    */
    int DisqueVirtuel::bd_rm(const std::string& p_Filename) {

        std::string path, nomSupprimer;
        splitDerniereNom(p_Filename, path, nomSupprimer);

        if (!nomExiste(p_Filename)){
            std::cout << "Le nom " << nomSupprimer << " n'existe pas dans le répertoire " << path << std::endl;
            return 0;
        }

        //on chercher où se trouve le répertoire/fichier qu'on veut supprimer.
        int iNodeParentNomSupprimer = trouveNbINode(path);
        int blocParentNomSupprimer = m_blockDisque.at(iNodeParentNomSupprimer + BASE_BLOCK_INODE).m_inode->st_block;

        //on cherche l'inode lui même du répertoire/fichier a supprimer
        int iNodeSupprimer = trouveNbINode(p_Filename);
        int blocSupprimer = m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_block;


        if (m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_mode == S_IFDIR) { //si c'est un répertoire
            if (m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_size == 2 * 28) { //et ne contient que les 2 répertoires  "." et ".."
                m_blockDisque.at(blocSupprimer).m_dirEntry.clear(); //on supprime "." et ".."
                m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_nlink = calculNombreLiens(iNodeSupprimer);

                // puisque maintenant st_nlink=0 on peut supprimer
                for (int i = 0; i < m_blockDisque.at(
                        blocParentNomSupprimer).m_dirEntry.size(); i++) { //on cherche où il se trouve dans son parent
                    if (m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.at(i)->m_filename == nomSupprimer) {
                        //on retire le nom du DirEntry du parent
                        m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.erase(m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.begin() + i);
                        //on met a jour les info du parent
                        miseAJourInodeParent(iNodeParentNomSupprimer);

                        // on libere l'inode et le bloc
                        libererInode(iNodeSupprimer);
                        miseAJourBitmapINode(iNodeSupprimer, true);
                        miseAJourBitmapBloc(blocSupprimer, true);

                        return 1;
                    }
                }

            } else {
                std::cout << "Le répertoire " << p_Filename << " n'est pas vide! Vous ne pouvez pas le supprimer." << std::endl;
                return 0;
            }
        } else {  //est un fichier
            m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_nlink = m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_nlink - 1;
            if (m_blockDisque.at(iNodeSupprimer + BASE_BLOCK_INODE).m_inode->st_nlink == 0) {
                for (int i = 0; i < m_blockDisque.at(
                        blocParentNomSupprimer).m_dirEntry.size(); i++) { //on cherche où il se trouve dans son parent
                    if (m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.at(i)->m_filename == nomSupprimer) {
                        //on retire le nom du DirEntry du parent
                        m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.erase(m_blockDisque.at(blocParentNomSupprimer).m_dirEntry.begin() + i);
                        //on met a jour les info du parent
                        miseAJourInodeParent(iNodeParentNomSupprimer);

                        // on libere l'inode
                        libererInode(iNodeSupprimer);
                        miseAJourBitmapINode(iNodeSupprimer, true);

                        return 1;
                    }
                }
            }
        }
    }

    /**
    * \brief fait "split" sur string pour chercher le nom du répertoire/ fichier qui sera cree (c.a.d on prends le DERNIERE nom doc/path)
    */
    void DisqueVirtuel::splitDerniereNom(const std::string cheminFichier, std::string& path, std::string& nomFichier){
        std::size_t found = cheminFichier.find_last_of("/");
        if (found == 0){
            path = cheminFichier.front();
            nomFichier = cheminFichier.substr(found+1);
        }
        else{
            path = cheminFichier.substr(0,found);
            nomFichier = cheminFichier.substr(found+1);
        }
    }

    /**
    * \brief cherche le nom du répertoire parent de tout le path passe en entree, c.a.d. le PREMIER répertoire du chemin passe
    */
    void DisqueVirtuel::splitPremierRepertoire(const std::string path, std::string& cherche, std::string& restant){
        std::size_t found = path.find_first_of("/");
        cherche = path.substr(0,found);
        restant = path.substr(found+1);
    }

    /**
    * \brief cree les répertoires . et .. et met à jour les infos de l’inode libre.
    */
    void DisqueVirtuel::creerRepertoireVide(size_t nbInodeParent, size_t nbiNodeLibre, size_t nbBlocLibre ){

        // le bloc contient la liste des dirEntry
        m_blockDisque.at(nbBlocLibre).m_type_donnees = S_IFDE;

        dirEntry *p1 = new dirEntry(nbiNodeLibre, ".");
        m_blockDisque.at(nbBlocLibre).m_dirEntry.push_back(p1);

        dirEntry *p2 = new dirEntry(nbInodeParent, "..");
        m_blockDisque.at(nbBlocLibre).m_dirEntry.push_back(p2);

        // on met a jour les attributs de l'inode libre
        m_blockDisque.at(nbiNodeLibre + BASE_BLOCK_INODE).m_inode->st_mode = S_IFDIR;
        m_blockDisque.at(nbiNodeLibre + BASE_BLOCK_INODE).m_inode->st_size = m_blockDisque.at(nbBlocLibre).m_dirEntry.size() * 28;
        m_blockDisque.at(nbiNodeLibre + BASE_BLOCK_INODE).m_inode->st_block = nbBlocLibre;
        m_blockDisque.at(nbiNodeLibre + BASE_BLOCK_INODE).m_inode->st_nlink = calculNombreLiens(nbiNodeLibre);

        miseAJourBitmapINode(nbiNodeLibre, false );
        miseAJourBitmapBloc(nbBlocLibre, false);
    }

    /**
    * \brief La méthode cherche le premier bloc libre
    * \return index du premier bloc libre
    */
    int DisqueVirtuel::trouveBlocLibre(){

        for(int i= 0; i < N_BLOCK_ON_DISK; i++){
            if (m_blockDisque.at(FREE_BLOCK_BITMAP).m_bitmap.at(i) == true){
                return i;
            }
        }
    }

    /**
    * \brief La méthode cherche le premier INode libre
    * \return index du premier INode libre
    */
    int DisqueVirtuel::trouveINodeLibre(){

        for(int i= 0; i < N_INODE_ON_DISK; i++){
            if (m_blockDisque.at(FREE_INODE_BITMAP).m_bitmap.at(i) == true){
                return i;
            }
        }
    }

    /**
    * \brief cherche le numero d'inode du répertoire/fichier en entree
    * \return le numero d'inode
    */
    int DisqueVirtuel::trouveNbINode(const std::string path){

        if (path == "/"){
            return ROOT_INODE;
        }

        std::string nomInodeCherche, ResteString;
        splitDerniereNom(path, ResteString, nomInodeCherche);

        for(int i = BASE_BLOCK_INODE; i < N_INODE_ON_DISK; i++){
            int blocDonnes = m_blockDisque.at(i).m_inode->st_block;
            for (int j = 0; j < m_blockDisque.at(blocDonnes).m_dirEntry.size(); j++){
                if (m_blockDisque.at(blocDonnes).m_dirEntry.at(j)->m_filename.compare(nomInodeCherche) == 0){
                    return  m_blockDisque.at(blocDonnes).m_dirEntry.at(j)->m_iNode; //on retourne le numero de l'inode requis.
                }
            }
        }
    }

    /**
    * \brief La méthode verifie si le répertoire/ chemin est valide ou pas
    * \return true pour valide, false sinon
    */
    bool DisqueVirtuel::repertoireExiste(const std::string nomPath){

        std::string copiePath = nomPath;
        if (copiePath.size() == 1 && copiePath.front() == '/')
        {
            return true;
        }
        if (copiePath.front() == '/'){
            copiePath.erase(0,1); // on delete le premier / qui c'est la racine et on recupere le nom des autres répertoires
        }

        int compt = std::count(copiePath.begin(),copiePath.end(), '/'); //on verifie combien de répertoire on cherche

        std::string dirCherche, ResteString;
        splitPremierRepertoire(copiePath, dirCherche, ResteString );

        for(int i = BASE_BLOCK_INODE; i < N_INODE_ON_DISK; i++){
            if (m_blockDisque.at(i).m_inode->st_mode == S_IFDIR){
                int blocDonnes = m_blockDisque.at(i).m_inode->st_block; //on recupere le numero du bloc de chaque inode S_IFDIR
                for (int j = 0; j < m_blockDisque.at(blocDonnes).m_dirEntry.size(); j++){
                    if (m_blockDisque.at(blocDonnes).m_dirEntry.at(j)->m_filename.compare(dirCherche) == 0){ //pour  chaque bloc on compare le nom avec dirCherche
                        compt--;
                        if (compt >= 0){
                            return repertoireExiste(ResteString); //s'il y a encore de répertoire pour chercher on fait un appel recursif
                        }
                        else{
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    /**
    * \brief La méthode verifie s'il existe deja un répertoire ou fichier avec ce nom dans le répertoire en question
    * \return true pour existe deja, false sinon
    */
    bool DisqueVirtuel::nomExiste(const std::string nom){

        std::string copiePath = nom;

        int compt = std::count(copiePath.begin(),copiePath.end(), '/'); //on verifie si le parent est le répertoire racine

        // si le parent n'est pas la racine on delete le premier / (racine) et on recupere le nom des autres répertoires
        if ( compt != 1 && copiePath.front() == '/'){
            copiePath.erase(0,1);
        }

        std::string dirParent, nomCherche;
        splitDerniereNom(copiePath, dirParent, nomCherche );

        int iNodeDirParent = trouveNbINode(dirParent);

        int blocDireParent = m_blockDisque.at(iNodeDirParent + BASE_BLOCK_INODE ).m_inode->st_block;

        for (int j = 0; j < m_blockDisque.at(blocDireParent).m_dirEntry.size(); j++){
            if (m_blockDisque.at(blocDireParent).m_dirEntry.at(j)->m_filename.compare(nomCherche) == 0){
                return true;
            }
        }
        return false;
    }

    /**
    * \brief Affiche message lorsqu'on relache ou saisie un Bloc
    */
    void DisqueVirtuel::afficherMessageBloc(bool commande, size_t nbBloc){
        if (commande == true){
            std:: cout << "UFS: Relache bloc " << nbBloc  << std::endl;
        }
        else{
            std:: cout << "UFS: saisir bloc " << nbBloc  << std::endl;
        }
    }

    /**
    * \brief Affiche message lorsqu'on relache ou saisie un Inode
    */
    void DisqueVirtuel::afficherMessageInode(bool commande, size_t nbINode){
        if (commande == true){
            std:: cout << "UFS: Relache i-node " << nbINode  << std::endl;
        }
        else{
            std:: cout << "UFS: saisir i-node " << nbINode  << std::endl;
        }
    }

    /**
    * \brief compte combien de fois l'inode est référencée
    * \return le nombre de liens de l'inode
    */
    int DisqueVirtuel::calculNombreLiens(size_t nbInode){

        size_t nbLiens = 0;

        if (m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_mode == S_IFREG){ //  si c'est un fichier:

            nbLiens = m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_nlink + 1;
        }
        else { //si c'est un répertoire:

            int bloc = m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_block; //on cherche le bloc correspondant au inode
            for (int i=0; i < m_blockDisque.at(bloc).m_dirEntry.size(); i++ ){
                int inodeReference = m_blockDisque.at(bloc).m_dirEntry.at(i)->m_iNode;
                if (m_blockDisque.at(inodeReference + BASE_BLOCK_INODE).m_inode->st_mode != S_IFREG ){
                    nbLiens++;
                }
            }
        }

        return nbLiens;
    }

    /**
    * \brief met a jour st_nlink et st_size de l’inode parent passe en entree
    */
    void DisqueVirtuel::miseAJourInodeParent(size_t nbInode){

        int bloc = m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_block;
        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_nlink = calculNombreLiens(nbInode);
        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_size = m_blockDisque.at(bloc).m_dirEntry.size() * 28;
    }

    /**
    * \brief met a jour le bitmapInode
    */
    void DisqueVirtuel::miseAJourBitmapINode(size_t nbInode, bool statut){
        m_blockDisque.at(FREE_INODE_BITMAP).m_bitmap.at(nbInode) = statut;
        afficherMessageInode(statut, nbInode );
    }

    /**
    * \brief met a jour le bitmapBloc
    */
    void DisqueVirtuel::miseAJourBitmapBloc(size_t nbBloc, bool statut){
        m_blockDisque.at(FREE_BLOCK_BITMAP).m_bitmap.at(nbBloc)= statut;
        afficherMessageBloc(statut, nbBloc );
    }

    /**
    * \brief Libère/réinitialise les attributs associés à l’inode.
    */
    void DisqueVirtuel::libererInode(size_t nbInode){

        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_nlink = 0;
        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_mode = 0;
        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_block = 0;
        m_blockDisque.at(nbInode + BASE_BLOCK_INODE).m_inode->st_size = 0;
    }
}//Fin du namespace