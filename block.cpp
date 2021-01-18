/**
 * \file block.cpp
 * \brief Implémentation d'un bloc.
 * \author ?
 * \version 0.1
 * \date nov-dec 2020
 *
 *  Travail pratique numero 3
 *
 */


#include "disqueVirtuel.h"
//vous pouvez inclure d'autres librairies si c'est necessaire
#include "block.h"

namespace TP3
{

    // Constructeur par défaut
    Block::Block():  m_type_donnees(),m_inode(), m_bitmap(), m_dirEntry(){
    }

    // Constructeur pour initialiser m_type_donnees
    Block::Block(size_t td): m_type_donnees(td), m_inode(), m_bitmap(), m_dirEntry()  {
    }

    // on va detruire les block dans le destructeur de DisqueVirtuel
    Block::~Block(){
    }


}//Fin du namespace




